#ifndef _DE_RENDERER_
#define _DE_RENDERER_

#include<vulkan/vulkan.hpp>
#include<vector>
#include<memory>
#include<optional>
#include "DE_Swapchain.hpp"
#include "DE_RenderTarget.hpp"

namespace de {
	class Device;
	class RenderPass;
	class CommandPool;
	class CommandBuffer;
	class Window;

	struct FrameData {
		uint32_t imageIndex;
		uint32_t frameIndex;
		vk::CommandBuffer commandBuffer;
	};

	class Renderer {
	public:
		Renderer(const Device& device, const RenderPass& renderPass, 
			Window& window, const CommandPool& commandPool,
			uint32_t minImageCount, uint32_t framesInFlight = 2);

		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = default;
		Renderer& operator=(Renderer&&) = default;

		std::optional<FrameData> beginFrame();

		void beginRenderPass(const FrameData& frameData, const RenderTarget& target,
			const std::vector<vk::ClearValue> clearValues);
		void endRenderPass(const FrameData& frameData);

		void endFrame();
		RenderTarget& getSwapchainRenderTarget() noexcept { return swapchainTarget_; }

		vk::Extent2D getSwapchainExtent() const noexcept {return swapchain_->getSwapchainExtent();}
		uint32_t getFramesInFlight() const noexcept { return framesInFlight_; }
		bool isFrameInfProgress() const noexcept {return isFrameStarted_;}

	private:
		void recreateSwapchain();
		void recreatePerImageSyncObjects();

		const Device& device_;
		const RenderPass& renderPass_;
		Window& window_;
		const CommandPool& commandPool_;
		std::optional<Swapchain> swapchain_;
		SwapchainRenderTarget swapchainTarget_;
		std::unique_ptr<CommandBuffer> commandBuffer_;
		uint32_t minImageCount_;

		uint32_t framesInFlight_;
		uint32_t currentFrameIndex_ = 0;
		uint32_t currentImageIndex_ = 0;
		bool isFrameStarted_ = false;

		std::vector<vk::UniqueFence> inFlightFences_;
		std::vector<vk::UniqueSemaphore> readyForRenderengSemaphores_;
		std::vector<vk::UniqueSemaphore> readyForPresentingSemaphores_;
		std::vector<vk::Fence> imagesInFlightFences_;
	};
}
#endif