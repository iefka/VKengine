#ifndef _DE_SWAPCHAIN_
#define _DE_SWAPCHAIN_


#include<vulkan/vulkan.hpp>
#include<vector>

namespace de {
	class Device;
	class RenderPass;

	struct gpuImage {
		vk::UniqueImage image;
		vk::UniqueDeviceMemory memory;
	};

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


	class Swapchain {
	public:
		class Builder {
		public:
			explicit Builder(const Device& device) : device_(device) {}

			//set image format and color space by surface format
			Builder& setImageFormatAndColorSpace(const vk::SurfaceFormatKHR& surfaceFormat);
			Builder& setSurface(const vk::SurfaceKHR& surface);
			Builder& setImageExtent(const vk::Extent2D& extent);
			Builder& setOldSwapchain(vk::SwapchainKHR oldSwapchain);
			
			// default value is 2
			Builder& setMinImageCount(const uint32_t minImageCount = 2);
			// default value is 1
			Builder& setImageArrayLayers(const uint32_t arrayLayers = 1);
			// default value is eColorAttachment
			Builder& setImageUsage(const vk::ImageUsageFlagBits& imageUsage = vk::ImageUsageFlagBits::eColorAttachment);
			// default value is eExclusive
			Builder& setImageSharingMode(const vk::SharingMode& sharingMode = vk::SharingMode::eExclusive);
			// default value is eIdentity
			Builder& setPreTransform(const vk::SurfaceTransformFlagBitsKHR& preTransform = vk::SurfaceTransformFlagBitsKHR::eIdentity);
			// default value is eOpaque
			Builder& setCompositeAlpha(const vk::CompositeAlphaFlagBitsKHR& comAlpha = vk::CompositeAlphaFlagBitsKHR::eOpaque);
			// default value is eFifo
			Builder& setPresentMode(const vk::PresentModeKHR& presentMode = vk::PresentModeKHR::eFifo );
			// default value is true;
			Builder& setClipped(const bool cliped = true);


			Swapchain build();

		private:
			const Device& device_;
			vk::SwapchainKHR oldSwapchain_;
			vk::SurfaceKHR surface_;
			vk::SurfaceFormatKHR surfaceFormat_;
			vk::Extent2D extent_;
			uint32_t minImageCount_;
			uint32_t arrayLayers_;
			vk::ImageUsageFlagBits	imageUsage_;
			vk::SharingMode sharingMode_;
			vk::SurfaceTransformFlagBitsKHR preTransform_;
			vk::CompositeAlphaFlagBitsKHR comAlpha_;
			vk::PresentModeKHR presentMode_;
			bool cliped_;
		};

		~Swapchain() = default;

		Swapchain(const Swapchain&) = delete;
		Swapchain& operator=(const Swapchain&) = delete;

		Swapchain(Swapchain&&) = default;
		Swapchain& operator=(Swapchain&&) = delete;

		uint32_t getImageCount() const noexcept { return static_cast<uint32_t>(imageViews_.size()); }
		vk::SwapchainKHR getHandle() const noexcept { return *swapchain_; }
		vk::Extent2D getSwapchainExtent() const noexcept{ return currentExtent_; }
		uint32_t getMinImageCount() const noexcept { return minImage_; }
		vk::ImageView getColorView(uint32_t index) const noexcept { return *imageViews_[index]; }
		vk::ImageView getDepthView() const noexcept {return *depthImageView_;}
		vk::Image getColorImage(uint32_t index) const noexcept { return colorImages_[index]; }
		vk::Image getDepthImage() const noexcept {return *depthImage_.image;}

	private:
		friend class Builder;
		explicit Swapchain(const Device& device,const vk::SwapchainCreateInfoKHR& scInfo);

		const Device& device_;
		vk::UniqueSwapchainKHR swapchain_;
		vk::Extent2D currentExtent_;

		// per-image real image count
		uint32_t minImage_;
		uint32_t imageCount_;
		std::vector<vk::Image> colorImages_;
		std::vector<vk::UniqueImageView> imageViews_;
		vk::UniqueImageView depthImageView_;
		gpuImage depthImage_;
	};

}


#endif 
