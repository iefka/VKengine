#ifndef _DE_RENDERER_
#define _DE_RENDERER_

#include<vulkan/vulkan.hpp>
#include<vector>

namespace de {
	class Device;
	class RenderPass;
	class Buffer;
	class CommandPool;
	class DescriptorPool;
	class DescriptorSetLayout;

	struct gpuImage {
		vk::UniqueImage image;
		vk::UniqueDeviceMemory memory;
	};

	vk::UniqueImageView createImageView(
		const Device& device,
		const vk::Image& image,
		const vk::Format& format,
		vk::ImageAspectFlagBits imageAspect = vk::ImageAspectFlagBits::eColor);


	

	std::vector<gpuImage> createImages(const de::Device& device,
		uint32_t imageCreationCount,
		const vk::Format& format,
		vk::ImageUsageFlags imageUsage,
		const vk::Extent2D& extent,
		vk::MemoryPropertyFlags memoryPropertyFlags = vk::MemoryPropertyFlagBits::eDeviceLocal,
		vk::ImageType imageType = vk::ImageType::e2D,
		uint32_t mipLevels = 1,
		uint32_t arrayLayers = 1,
		vk::SampleCountFlagBits sampleCount = vk::SampleCountFlagBits::e1,
		vk::ImageTiling imageTiling = vk::ImageTiling::eOptimal,
		vk::ImageLayout initialLayout = vk::ImageLayout::eUndefined,
		vk::SharingMode sharing = vk::SharingMode::eExclusive,
		vk::ImageCreateFlagBits flags = {}
		);

	gpuImage createDepthImage(
		const de::Device& device,
		const vk::Format& depthFormat,
		const vk::Extent2D& extent,
		vk::ImageUsageFlags additionalUsages = {}, 
		vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1,
		uint32_t arrayLayers = 1);

	std::vector<vk::UniqueImageView> createImageViews(
		const Device& device,
		const vk::SwapchainKHR& swapchain,
		const vk::Format& format);

	std::vector<vk::UniqueFramebuffer> createFramebuffers(
		const Device& device,
		const std::vector<vk::UniqueImageView>& imageViews,
		const vk::UniqueImageView& depthImageView,
		const vk::Extent2D& extent,
		const vk::RenderPass& renderPass);

	struct FrameData
	{
		std::uint32_t swapchainImageIndex;
		std::uint32_t inFlightIndex;

		const vk::UniqueFramebuffer& framebuffer;
		const vk::UniqueFence& inFlightFence;
		const vk::UniqueSemaphore& readyForRenderingSemaphore;
		const vk::UniqueSemaphore& readyForPresentingSemaphore;
		const vk::CommandBuffer& commandBuffer;
		const vk::DescriptorSet& descriptor ;
	};

	class Swapchain {
	public:

		class Builder {
		public:
			//урок 21а сделать получение данных через свопчейн, для этого передать рендерпас и дальше котакбасс
			explicit Builder(const Device& device,const RenderPass& renderPass) : device_(device),renderPass_(renderPass) {}

			//set image format and color space by surface format
			Builder& setImageFormatAndColorSpace(const vk::SurfaceFormatKHR& surfaceFormat);
			Builder& setSurface(const vk::SurfaceKHR& surface);
			Builder& setImageExtent(const vk::Extent2D& extent);
			
			// default value is 2
			Builder& setMinImageCount(const uint32_t minImageCount);
			// default value is 1
			Builder& setImageArrayLayers(const uint32_t arrayLayers);
			// default value is eColorAttachment
			Builder& setImageUsage(const vk::ImageUsageFlagBits& imageUsage);
			// default value is eExclusive
			Builder& setImageSharingMode(const vk::SharingMode& sharingMode);
			// default value is eIdentity
			Builder& setPreTransform(const vk::SurfaceTransformFlagBitsKHR& preTransform);
			// default value is eOpaque
			Builder& setCompositeAlpha(const vk::CompositeAlphaFlagBitsKHR& comAlpha);
			// default value is eFifo
			Builder& setPresentMode(const vk::PresentModeKHR& presentMode);
			// default value is true;
			Builder& setClipped(const bool cliped);


			Swapchain build();

		private:
			const Device& device_;
			const RenderPass& renderPass_;
			std::vector<CommandBuffer> commandBuffers;
			vk::SurfaceKHR surface_;
			vk::SurfaceFormatKHR surfaceFormat_;
			vk::Extent2D extent_;
			uint32_t minImageCount_ = 2;
			uint32_t arrayLayers_ = 1;
			vk::ImageUsageFlagBits	imageUsage_				= vk::ImageUsageFlagBits::eColorAttachment;
			vk::SharingMode sharingMode_					= vk::SharingMode::eExclusive;
			vk::SurfaceTransformFlagBitsKHR preTransform_	= vk::SurfaceTransformFlagBitsKHR::eIdentity;
			vk::CompositeAlphaFlagBitsKHR comAlpha_			= vk::CompositeAlphaFlagBitsKHR::eOpaque;
			vk::PresentModeKHR presentMode_					= vk::PresentModeKHR::eFifo;
			bool cliped_ = true;
		};

		Swapchain() = delete;

		Swapchain(Swapchain&) = default;
		Swapchain& operator=(Swapchain&) = default;

		Swapchain(Swapchain&&) = default;
		Swapchain& operator=(Swapchain&&) = default;

		const vk::UniqueSwapchainKHR& getSwapchain() const noexcept { return swapchain_; }
		void createFrameResources(const CommandPool& cmdPool);
		void createFrameDescriptorSets(const DescriptorPool& descriptorPool, const DescriptorSetLayout& layout, const std::vector<std::unique_ptr<Buffer>>& buffers);
		FrameData getNextFrame();

	private:
		explicit Swapchain(const Device& device,const RenderPass& renderPass,const vk::SwapchainCreateInfoKHR& scInfo);

		const Device& device_;
		const RenderPass& renderPass_;
		vk::UniqueSwapchainKHR swapchain_;

		std::unique_ptr<CommandBuffer> frameCommandBuffer_;
		std::vector<vk::DescriptorSet> frameDescriptorSets_;

		std::uint32_t maxImagesInFlight_;
		std::uint32_t currentFrameIndex_ = 0;
		std::vector< vk::UniqueImageView > imageViews_;
		vk::UniqueImageView depthImageView_;
		gpuImage depthImage_;
		std::vector< vk::UniqueFramebuffer > framebuffers_;
		std::vector< vk::UniqueFence > inFlightFences_;
		std::vector< vk::UniqueSemaphore > readyForRenderingSemaphores_;
		std::vector< vk::UniqueSemaphore > readyForPresentingSemaphores_;
	};


	

}


#endif 
