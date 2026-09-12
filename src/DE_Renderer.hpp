#ifndef _DE_RENDERER_
#define _DE_RENDERER_

#include<vulkan/vulkan.hpp>
#include<vector>
#include<memory>
#include<optional>
#include "DE_Swapchain.hpp"
#include "DE_RenderTarget.hpp"
#include"DE_IRenderPass.hpp"

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


	class SwapchainRenderTarget : public RenderTarget {
	public:

		// swapchain pointer is nullptr on creation, use rebind() befor use!
		SwapchainRenderTarget() = default;
		void rebind(const Swapchain& swapchain) { swapchain_ = &swapchain; }

		vk::Framebuffer getFramebuffer(uint32_t imageIndex) const override {
			return swapchain_->getFrameBuffer(imageIndex);
		}
		vk::RenderPass getRenderPass() const override {
			return swapchain_->getRenderPass();
		}
		vk::Extent2D getExtent() const override {
			return swapchain_->getSwapchainExtent();
		}
	private:
		const Swapchain* swapchain_ = nullptr;
	};

	class Renderer {
	public:
		Renderer(const Device& device, 
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

		void renderFrame();

		void endRenderPass(const FrameData& frameData);
		void endFrame();

		void registerPass(IRenderPass* pass){
			passes_.push_back(pass);
		}
		void unregisterPass(IRenderPass* pass) {
			auto removed = std::ranges::remove(passes_, pass);
			passes_.erase(removed.begin(), removed.end());
		}

		RenderTarget& getSwapchainRenderTarget() noexcept { return swapchainTarget_; }

		const RenderPass& getMainRenderPass() { return *renderPass_; }

		vk::Extent2D getSwapchainExtent() const noexcept {return swapchain_->getSwapchainExtent();}
		uint32_t getFramesInFlight() const noexcept { return framesInFlight_; }
		bool isFrameInfProgress() const noexcept {return isFrameStarted_;}

	private:
		void recreateSwapchain();
		void recreatePerImageSyncObjects();

		const Device& device_;
		//const RenderPass& renderPass_;
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

		
		std::unique_ptr<RenderPass> renderPass_;
		std::vector<IRenderPass*> passes_;
		std::vector<vk::UniqueFence> inFlightFences_;
		std::vector<vk::UniqueSemaphore> readyForRenderengSemaphores_;
		std::vector<vk::UniqueSemaphore> readyForPresentingSemaphores_;
		std::vector<vk::Fence> imagesInFlightFences_;
	};
}
#endif