#include"DE_Device.hpp"
#include"DE_CommandBuffers.hpp"
#include"DE_RenderPass.hpp"
#include"DE_Window.hpp"
#include<limits>
#include "DE_Renderer.hpp"

namespace de {
	Renderer::Renderer(const Device& device, 
		Window& window, const CommandPool& commandPool,
		uint32_t minImageCount, uint32_t framesInFlight) 
		:device_{ device },
		commandPool_{ commandPool },
		window_{ window },
		minImageCount_{ minImageCount }, 
		framesInFlight_{framesInFlight} {


		renderPass_ = std::make_unique<de::RenderPass>(
			de::RenderPass::Builder{ device_ }
			.setColorAttachment(window_.getSurfaceFormat().format)
			.setDepthAttachment(vk::Format::eD32Sfloat)
			.build()
		);

		swapchain_.emplace(
			Swapchain::Builder{ device_,*renderPass_ }
			.setClipped(true)
			.setSurface(*window_.getSurface())
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setImageArrayLayers(1)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setImageFormatAndColorSpace(window_.getSurfaceFormat())
			.setImageExtent(window_.getExtent())
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setMinImageCount(minImageCount_)
			.setPresentMode(vk::PresentModeKHR::eFifo)
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.build()
		);

		swapchainTarget_.rebind(*swapchain_);

		commandBuffer_ = std::make_unique<CommandBuffer>(
			commandPool_.allocate(vk::CommandBufferLevel::ePrimary, framesInFlight_)	
		);

		const auto& logicalDevice = device_.getLogicalDevice();

		for (uint32_t i = 0; i < framesInFlight_; i++) {
			inFlightFences_.push_back(
				logicalDevice.createFenceUnique(
					vk::FenceCreateInfo{}
					.setFlags(vk::FenceCreateFlagBits::eSignaled)
				)
			);

			readyForRenderengSemaphores_.push_back(
				logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo{})
			);
		}
		recreatePerImageSyncObjects();
	}

	void Renderer::recreatePerImageSyncObjects(){
		const auto& logicalDevice = device_.getLogicalDevice();
		const auto imageCount = swapchain_->getImageCount();

		readyForPresentingSemaphores_.clear();
		readyForPresentingSemaphores_.resize(imageCount);
		for (uint32_t i = 0; i < imageCount; ++i) {
			readyForPresentingSemaphores_[i] = logicalDevice.createSemaphoreUnique(vk::SemaphoreCreateInfo{});
		}

		imagesInFlightFences_.assign(imageCount, vk::Fence{});
	}

	void Renderer::recreateSwapchain() {

		auto extent = window_.getExtent();
		while (extent.width == 0 && extent.height == 0) {
			glfwWaitEvents();
			extent = window_.getExtent();
		}
		device_.getLogicalDevice().waitIdle();

		const auto oldHandle = swapchain_->getHandle();

		swapchain_.emplace(
			Swapchain::Builder{ device_,*renderPass_ }
			.setClipped(true)
			.setSurface(*window_.getSurface())
			.setCompositeAlpha(vk::CompositeAlphaFlagBitsKHR::eOpaque)
			.setImageArrayLayers(1)
			.setImageSharingMode(vk::SharingMode::eExclusive)
			.setImageFormatAndColorSpace(window_.getSurfaceFormat())
			.setImageExtent(window_.getExtent())
			.setImageUsage(vk::ImageUsageFlagBits::eColorAttachment)
			.setMinImageCount(minImageCount_)
			.setPresentMode(vk::PresentModeKHR::eFifo)
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setOldSwapchain(oldHandle)
			.build()
		);
		swapchainTarget_.rebind(*swapchain_);

		recreatePerImageSyncObjects();
	}

	std::optional<FrameData> Renderer::beginFrame() {
		if (isFrameStarted_) {
			throw std::runtime_error("beginFrame called while frame already in progress");
		}

		const auto& logicalDevice = device_.getLogicalDevice();

		logicalDevice.waitForFences(
			*inFlightFences_[currentFrameIndex_], true, std::numeric_limits<uint64_t>::max()
		);

		uint32_t imageIndex;

		try {
			auto acquired = logicalDevice.acquireNextImageKHR(
				swapchain_->getHandle(),
				std::numeric_limits<uint64_t>::max(),
				*readyForRenderengSemaphores_[currentFrameIndex_],
				nullptr);
			imageIndex = acquired.value;
		}catch(vk::OutOfDateKHRError){
			recreateSwapchain();
			return std::nullopt;
		}

		if (imagesInFlightFences_[imageIndex]) {
			logicalDevice.waitForFences(
				imagesInFlightFences_[imageIndex], true,
				std::numeric_limits<uint64_t>::max()
			);
		}
		imagesInFlightFences_[imageIndex] =
			*inFlightFences_[currentFrameIndex_];
		logicalDevice.resetFences(*inFlightFences_[currentFrameIndex_]);

		currentImageIndex_ = imageIndex;
		isFrameStarted_ = true;

		auto commandBuffer = commandBuffer_->handle(currentFrameIndex_);
		commandBuffer.begin(vk::CommandBufferBeginInfo{});

		return FrameData{
			imageIndex,
			currentFrameIndex_,
			commandBuffer
		};

	}

	void Renderer::beginRenderPass(const FrameData& frameData, const RenderTarget& target,
		const std::vector<vk::ClearValue> clearValues) {

		if (!isFrameStarted_) {
			throw std::runtime_error("beginRenderPass called outside of a frame");
		}

		const auto beginRenderPassInfo = vk::RenderPassBeginInfo{}
			.setClearValues(clearValues)
			.setRenderPass(target.getRenderPass())
			.setFramebuffer(target.getFramebuffer(frameData.imageIndex))
			.setRenderArea(vk::Rect2D{ vk::Offset2D{0, 0}, target.getExtent() });

		frameData.commandBuffer.beginRenderPass(beginRenderPassInfo, vk::SubpassContents::eInline);
		
		const auto extent = target.getExtent();
		frameData.commandBuffer.setViewport(0, vk::Viewport{}
			.setX(0.f)
			.setY(0.f)
			.setWidth(static_cast<float>(extent.width))
			.setHeight(static_cast<float>(extent.height))
			.setMinDepth(0.0f)
			.setMaxDepth(1.f)
		);
		frameData.commandBuffer.setScissor(0, vk::Rect2D{ {0,0},extent });
	}
	void Renderer::renderFrame(){
		
		auto frameOptimal = beginFrame();
		if (!frameOptimal) { return; }
		auto& frame = *frameOptimal;

		for (auto& pass : passes_) {

			beginRenderPass(frame, pass->getRenderTarget(), pass->getClearValues());
			pass->record(frame);
			endRenderPass(frame);
		}

		endFrame();

	}
	void Renderer::endRenderPass(const FrameData& frameData) {
		if (!isFrameStarted_) {
			throw std::runtime_error("endRenderPass called outside of a frame");
		}
		frameData.commandBuffer.endRenderPass();
	}

	void Renderer::endFrame() {
		if (!isFrameStarted_) {
			throw std::runtime_error("endFrame while no frame in progress");
		}
		auto commandBuffer = commandBuffer_->handle(currentFrameIndex_);
		commandBuffer.end();

		const vk::PipelineStageFlags waitStages[] = {
			vk::PipelineStageFlagBits::eColorAttachmentOutput
		};
		const auto submitInfo = vk::SubmitInfo{}
			.setCommandBuffers(commandBuffer)
			.setWaitDstStageMask(waitStages)
			.setWaitSemaphores(*readyForRenderengSemaphores_[currentFrameIndex_])
			.setSignalSemaphores(*readyForPresentingSemaphores_[currentImageIndex_]);

		device_.getGraphicsQueue().queue.submit(submitInfo, *inFlightFences_[currentFrameIndex_]);
		
		//continue life for presentInfo
		const auto swapchainHandle = swapchain_->getHandle();
		
		const auto presentInfo = vk::PresentInfoKHR{}
			.setSwapchains(swapchainHandle)
			.setImageIndices(currentImageIndex_)
			.setWaitSemaphores(*readyForPresentingSemaphores_[currentImageIndex_]);

		vk::Result presentResult = vk::Result::eSuccess;
		try {
			presentResult = device_.getPresentQueue().queue.presentKHR(presentInfo);
		}catch(const vk::OutOfDateKHRError&){
			presentResult = vk::Result::eErrorOutOfDateKHR;
		}

		isFrameStarted_ = false;
		currentFrameIndex_ = (currentFrameIndex_ + 1) % framesInFlight_;

		const bool needsRecreate = presentResult == vk::Result::eSuboptimalKHR
			|| presentResult == vk::Result::eErrorOutOfDateKHR
			|| window_.isResized();

		if (needsRecreate) {
			if (window_.isResized()) window_.endResize();
			recreateSwapchain();
		}
	}
}