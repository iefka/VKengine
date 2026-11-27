#include "DE_Renderer.hpp"
#include"DE_Device.hpp"
#include"DE_CommandBuffers.hpp"
#include"DE_RenderPass.hpp"
#include"DE_Window.hpp"
#include<limits>


namespace de {
	Renderer::Renderer(const Device& device, 
		Window& window, const CommandPool& commandPool,
		uint32_t minImageCount, uint32_t framesInFlight) 
		:device_{ device },
		window_{ window },
		commandPool_{ commandPool },
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
			.setPresentMode(vk::PresentModeKHR::eMailbox)
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.build()
		);

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

	void Renderer::transitionImage(vk::CommandBuffer cmd, vk::Image image,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess,
        vk::PipelineStageFlags dstStage, vk::AccessFlags dstAccess,
        vk::ImageAspectFlags aspect){
		const auto barrier = vk::ImageMemoryBarrier{}
			.setOldLayout(oldLayout)
			.setNewLayout(newLayout)
			.setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
			.setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
			.setImage(image)
			.setSrcAccessMask(srcAccess)
			.setDstAccessMask(dstAccess)
			.setSubresourceRange(vk::ImageSubresourceRange{}
				.setAspectMask(aspect)
				.setBaseMipLevel(0).setLevelCount(1)
				.setBaseArrayLayer(0).setLayerCount(1));

		cmd.pipelineBarrier(srcStage, dstStage, {}, nullptr, nullptr, barrier);
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
			.setPresentMode(vk::PresentModeKHR::eMailbox)
			.setPreTransform(vk::SurfaceTransformFlagBitsKHR::eIdentity)
			.setOldSwapchain(oldHandle)
			.build()
		);

		recreatePerImageSyncObjects();
	}

	std::optional<FrameData> Renderer::beginFrame() {
		if (isFrameStarted_) {
			throw std::runtime_error("beginFrame called while frame already in progress");
		}

		const auto& logicalDevice = device_.getLogicalDevice();

		[[maybe_unused]] auto result = logicalDevice.waitForFences(
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
			result = logicalDevice.waitForFences(
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

		transitionImage(commandBuffer, swapchain_->getColorImage(imageIndex),
			vk::ImageLayout::eUndefined, vk::ImageLayout::eColorAttachmentOptimal,
			vk::PipelineStageFlagBits::eTopOfPipe, {},
			vk::PipelineStageFlagBits::eColorAttachmentOutput,
			vk::AccessFlagBits::eColorAttachmentWrite,
			vk::ImageAspectFlagBits::eColor);

		transitionImage(commandBuffer, swapchain_->getDepthImage(),
			vk::ImageLayout::eUndefined, vk::ImageLayout::eDepthAttachmentOptimal,
			vk::PipelineStageFlagBits::eTopOfPipe, {},
			vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eLateFragmentTests,
			vk::AccessFlagBits::eDepthStencilAttachmentRead | vk::AccessFlagBits::eDepthStencilAttachmentWrite,
			vk::ImageAspectFlagBits::eDepth);

		return FrameData{
			imageIndex,
			currentFrameIndex_,
			commandBuffer
		};

	}

	void Renderer::beginRendering(const FrameData& frameData, IRenderPass& pass) {

		if (!isFrameStarted_) {
			throw std::runtime_error("beginRendering called outside of a frame");
		}

		const auto extent = swapchain_->getSwapchainExtent();

		const auto color = pass.colorClearValue();
		const auto colorAttInfo = vk::RenderingAttachmentInfo{}
			.setClearValue(color.clear)
			.setLoadOp(color.loadOp)
			.setStoreOp(color.storeOp)
			.setImageLayout(vk::ImageLayout::eColorAttachmentOptimal)
			.setImageView(swapchain_->getColorView(frameData.imageIndex));

		const auto depth = pass.depthClearValue();
		auto depthAttInfo = vk::RenderingAttachmentInfo{};
		if(depth){
			depthAttInfo
				.setClearValue(depth->clear)
				.setLoadOp(depth->loadOp)
				.setStoreOp(depth->storeOp)
				.setImageLayout(vk::ImageLayout::eDepthAttachmentOptimal)
				.setImageView(swapchain_->getDepthView());
		}

		const auto renderingInfo = vk::RenderingInfo{}
			.setColorAttachments(colorAttInfo)
			.setPDepthAttachment(depth? &depthAttInfo : nullptr)
			.setRenderArea(vk::Rect2D{{0,0},{extent}})
			.setLayerCount(1);

		frameData.commandBuffer.beginRendering(renderingInfo);
		
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

		for (auto& step : computeSteps_) {
			step->dispatch(frame.commandBuffer, frame.frameIndex);
		}

		for (auto& pass : passes_) {

			beginRendering(frame, *pass);
			pass->record(frame);
			endRendering(frame);
		}

		transitionImage(frame.commandBuffer, swapchain_->getColorImage(frame.imageIndex),
			vk::ImageLayout::eColorAttachmentOptimal, vk::ImageLayout::ePresentSrcKHR,
			vk::PipelineStageFlagBits::eColorAttachmentOutput, vk::AccessFlagBits::eColorAttachmentWrite,
			vk::PipelineStageFlagBits::eBottomOfPipe, {},
			vk::ImageAspectFlagBits::eColor);

		endFrame();

	}
	void Renderer::endRendering(const FrameData& frameData) {
		if (!isFrameStarted_) {
			throw std::runtime_error("endRenderPass called outside of a frame");
		}
		frameData.commandBuffer.endRendering();
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