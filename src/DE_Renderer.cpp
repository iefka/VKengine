//std
#include<limits>
//de
#include "DE_Device.hpp"
#include"DE_Memory.hpp"
#include"DE_Descriptors.hpp"
#include"DE_CommandBuffers.hpp"
#include"DE_RenderPass.hpp"
#include "DE_Renderer.hpp"
#include"Utility/DE_Utility.hpp"

namespace de {
	Swapchain::Builder& Swapchain::Builder::setImageFormatAndColorSpace(const vk::SurfaceFormatKHR& surfaceFormat) {

		surfaceFormat_ = surfaceFormat;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setSurface(const vk::SurfaceKHR& surface)
	{
		surface_ = surface;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setImageExtent(const vk::Extent2D& extent) {
		extent_ = extent;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setMinImageCount(const uint32_t minImageCount) {
		minImageCount_ = minImageCount;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setImageArrayLayers(const uint32_t arrayLayers) {
		arrayLayers_ = arrayLayers;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setImageUsage(const vk::ImageUsageFlagBits& imageUsage) {
		imageUsage_ = imageUsage;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setImageSharingMode(const vk::SharingMode& sharingMode) {
		sharingMode_ = sharingMode;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setPreTransform(const vk::SurfaceTransformFlagBitsKHR& preTransform) {
		preTransform_ = preTransform;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setCompositeAlpha(const vk::CompositeAlphaFlagBitsKHR& comAlpha) {
		comAlpha_ = comAlpha;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setPresentMode(const vk::PresentModeKHR& presentMode) {
		presentMode_ = presentMode;
		return *this;
	}
	Swapchain::Builder& Swapchain::Builder::setClipped(const bool cliped) {
		cliped_ = cliped;
		return *this;
	}


	Swapchain Swapchain::Builder::build() {

		const auto createInfo = vk::SwapchainCreateInfoKHR{}
			.setSurface(surface_)
			.setMinImageCount(minImageCount_)
			.setImageFormat(surfaceFormat_.format)
			.setImageColorSpace(surfaceFormat_.colorSpace)
			.setImageExtent(extent_)
			.setImageArrayLayers(arrayLayers_)
			.setImageUsage(imageUsage_)
			.setImageSharingMode(sharingMode_)
			.setPreTransform(preTransform_)
			.setCompositeAlpha(comAlpha_)
			.setPresentMode(presentMode_)
			.setClipped(cliped_);

		return Swapchain(device_,renderPass_, createInfo);
	}


	Swapchain::Swapchain(const Device& device,const RenderPass& renderPass, const vk::SwapchainCreateInfoKHR& scInfo):device_(device),renderPass_(renderPass),
		maxImagesInFlight_{scInfo.minImageCount}{
		const auto& logicalDevice = device_.getLogicalDevice();

		depthImage_ = createDepthImage(device_,
			vk::Format::eD32Sfloat,
			scInfo.imageExtent);

		depthImageView_ = createImageView(device_, *depthImage_.image, vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth);

		swapchain_ = logicalDevice.createSwapchainKHRUnique(scInfo);
		imageViews_ = createImageViews(device_, *swapchain_, scInfo.imageFormat);
		framebuffers_ = createFramebuffers(device_, imageViews_, depthImageView_, scInfo.imageExtent, *renderPass.getRenderPass());
		
		for (std::uint32_t i = 0; i < maxImagesInFlight_; ++i)
		{
			inFlightFences_.push_back(logicalDevice.createFenceUnique(
				vk::FenceCreateInfo{}.setFlags(vk::FenceCreateFlagBits::eSignaled)
			));
			readyForRenderingSemaphores_.push_back(logicalDevice.createSemaphoreUnique(
				vk::SemaphoreCreateInfo{}
			));
			readyForPresentingSemaphores_.push_back(logicalDevice.createSemaphoreUnique(
				vk::SemaphoreCreateInfo{}
			));
		}
	}

	void Swapchain::createFrameResources(const CommandPool& cmdPool){
				
		frameCommandBuffer_ = std::make_unique<CommandBuffer>(
			cmdPool.allocate(vk::CommandBufferLevel::ePrimary, maxImagesInFlight_)
		);
	}

	//creating descripror sets for each frame
	void Swapchain::createFrameDescriptorSets(const DescriptorPool& descriptorPool, const DescriptorSetLayout& layout,const std::vector<std::unique_ptr<Buffer>>& buffers) {
		
		
		frameDescriptorSets_.resize(maxImagesInFlight_);
		for (size_t i = 0; i < frameDescriptorSets_.size(); i++) {
			auto bufferInfo = buffers[i]->getDescriptorInfo();
			frameDescriptorSets_[i] = de::DescriptorWriter(device_, descriptorPool, layout)
				.writeBuffer(0, &bufferInfo)
				.build();
		}
	}

	FrameData Swapchain::getNextFrame()
	{
		const auto& logicalDevice = device_.getLogicalDevice();

		const auto result = logicalDevice.waitForFences(
			*inFlightFences_[currentFrameIndex_],
			true,
			std::numeric_limits<uint64_t>::max());
		logicalDevice.resetFences(*inFlightFences_[currentFrameIndex_]);

		auto imageIndex = logicalDevice.acquireNextImageKHR(*swapchain_,
			std::numeric_limits<uint64_t>::max(),
			*readyForRenderingSemaphores_[currentFrameIndex_]).value;

		

		 auto frame = FrameData{
			imageIndex,
			currentFrameIndex_,
			framebuffers_[currentFrameIndex_],
			inFlightFences_[currentFrameIndex_],
			readyForRenderingSemaphores_[currentFrameIndex_],
			readyForPresentingSemaphores_[imageIndex],
			frameCommandBuffer_->handle(currentFrameIndex_),
			frameDescriptorSets_[currentFrameIndex_]


		};
		currentFrameIndex_ = ++currentFrameIndex_ % maxImagesInFlight_;
		return frame;    
	}

	std::vector<vk::UniqueFramebuffer> createFramebuffers(
		const Device& device,
		const std::vector<vk::UniqueImageView>& imageViews,
		const vk::UniqueImageView& depthImageView,
		const vk::Extent2D& extent,
		const vk::RenderPass& renderPass) {

		const auto& logicalDevice = device.getLogicalDevice();

		std::vector<vk::UniqueFramebuffer> framebuffers;
		for (const auto& view : imageViews) {
			std::array<vk::ImageView, 2> attachments = { *view, *depthImageView };

			const auto framebufferInfo = vk::FramebufferCreateInfo{}
				.setRenderPass(renderPass)
				.setAttachments(attachments)
				.setWidth(extent.width)
				.setHeight(extent.height)
				.setLayers(1);

			framebuffers.emplace_back(logicalDevice.createFramebufferUnique(framebufferInfo));
		}

		return framebuffers;
	}

	vk::UniqueImageView createImageView(
		const Device& device,
		const vk::Image& image,
		const vk::Format& format,
		vk::ImageAspectFlagBits imageAspect) {

		const auto& logicalDevice = device.getLogicalDevice();

		const auto subresourceRange = vk::ImageSubresourceRange{}
			.setAspectMask(imageAspect)
			.setBaseMipLevel(0)
			.setLevelCount(1)
			.setBaseArrayLayer(0)
			.setLayerCount(1);

		const auto imageInfo = vk::ImageViewCreateInfo{}
			.setImage(image)
			.setViewType(vk::ImageViewType::e2D)
			.setFormat(format)
			.setSubresourceRange(subresourceRange);

		return logicalDevice.createImageViewUnique(imageInfo);
	}


	std::vector<gpuImage> createImages(const de::Device& device,
		uint32_t imageCreationCount,
		const vk::Format& format,
		vk::ImageUsageFlags imageUsage,
		const vk::Extent2D& extent,
		vk::MemoryPropertyFlags memoryPropertyFlags,
		vk::ImageType imageType,
		uint32_t mipLevels,
		uint32_t arrayLayers,
		vk::SampleCountFlagBits sampleCount,
		vk::ImageTiling imageTiling,
		vk::ImageLayout initialLayout,
		vk::SharingMode sharing,
		vk::ImageCreateFlagBits flags){

		const auto& logicalDevice = device.getLogicalDevice();
		const auto& physicalDevice = device.getPhysicalDevice();

		std::vector<gpuImage> result(imageCreationCount);

		const auto imageInfo = vk::ImageCreateInfo{}
			.setFlags(flags)
			.setUsage(imageUsage)
			.setArrayLayers(arrayLayers)
			.setImageType(imageType)
			.setMipLevels(mipLevels)
			.setExtent({ extent.width,extent.height, 1 })
			.setFormat(format)
			.setTiling(imageTiling)
			.setSamples(sampleCount)
			.setInitialLayout(initialLayout)
			.setSharingMode(sharing);
			
		for (uint32_t i = 0; i < imageCreationCount; ++i){
			result[i].image = logicalDevice.createImageUnique(imageInfo);

			const auto memoryReq = logicalDevice.getImageMemoryRequirements(*result[i].image);
			const auto memoryProp = physicalDevice.getMemoryProperties();

			uint32_t memoryIndex = utl::getSuitableMemoryIndex(memoryProp, memoryReq.memoryTypeBits, memoryPropertyFlags);

			const auto allocInfo = vk::MemoryAllocateInfo{}
				.setMemoryTypeIndex(memoryIndex)
				.setAllocationSize(memoryReq.size);

			result[i].memory = logicalDevice.allocateMemoryUnique(allocInfo);
			logicalDevice.bindImageMemory(*result[i].image, *result[i].memory, 0u);
		}
		return result;
	}

	gpuImage createDepthImage(const de::Device& device,
		const vk::Format& depthFormat,
		const vk::Extent2D& extent,
		vk::ImageUsageFlags additionalUsages,
		vk::SampleCountFlagBits samples,
		uint32_t arrayLayers){

			// Формируем флаги использования: всегда нужен DepthStencilAttachment
			vk::ImageUsageFlags usage = vk::ImageUsageFlagBits::eDepthStencilAttachment | additionalUsages;
			auto images = createImages(
				device,
				1,
				depthFormat,
				usage,
				extent);
			return std::move(images[0]);
	}
	

	std::vector<vk::UniqueImageView> createImageViews(
		const Device& device,
		const vk::SwapchainKHR& swapchain,
		const vk::Format& format) {

		const auto& logicalDevice = device.getLogicalDevice();
		auto swapchainImages = logicalDevice.getSwapchainImagesKHR(swapchain);

		std::vector< vk::UniqueImageView > swapChainImageViews;
		for (const auto& image : swapchainImages) {

			swapChainImageViews.push_back(
				createImageView(device, image, format)
			);
		}
		return swapChainImageViews;
	}
}

