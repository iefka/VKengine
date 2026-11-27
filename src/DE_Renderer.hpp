#ifndef _DE_RENDERER_
#define _DE_RENDERER_

#include<vulkan/vulkan.hpp>
#include<vector>
#include<memory>
#include<optional>
#include "DE_Swapchain.hpp"
#include"DE_IRenderPass.hpp"
#include"DE_IComputeStep.hpp"

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
		Renderer(const Device& device, 
			Window& window, const CommandPool& commandPool,
			uint32_t minImageCount, uint32_t framesInFlight = 2);

		~Renderer() = default;

		Renderer(const Renderer&) = delete;
		Renderer& operator=(const Renderer&) = delete;
		Renderer(Renderer&&) = delete;
		Renderer& operator=(Renderer&&) = delete;

		std::optional<FrameData> beginFrame();

		void beginRendering(const FrameData& frameData, IRenderPass& pass);

		void renderFrame();

		void endRendering(const FrameData& frameData);
		void endFrame();

		void registerPass(IRenderPass* pass){
			passes_.push_back(pass);
		}
		void unregisterPass(IRenderPass* pass) {
			auto removed = std::ranges::remove(passes_, pass);
			passes_.erase(removed.begin(), removed.end());
		}
		void registerCompute(IComputeStep* compute) {
			computeSteps_.push_back(compute);
		}
		void unregisterCompute(IComputeStep* compute) {
			auto removed = std::ranges::remove(computeSteps_, compute);
			computeSteps_.erase(removed.begin(), removed.end());
		}

		const RenderPass& getMainRenderPass() { return *renderPass_; }
		RenderPass* getMainRenderPassPtr() { return renderPass_.get(); }

		const Swapchain& getSwapchain()const noexcept { return swapchain_.value(); }
		vk::Extent2D getSwapchainExtent() const noexcept {return swapchain_->getSwapchainExtent();}
		uint32_t getFramesInFlight() const noexcept { return framesInFlight_; }
		bool isFrameInfProgress() const noexcept {return isFrameStarted_;}

	private:
		void recreateSwapchain();
		void recreatePerImageSyncObjects();
		void transitionImage(vk::CommandBuffer cmd, vk::Image image,
        vk::ImageLayout oldLayout, vk::ImageLayout newLayout,
        vk::PipelineStageFlags srcStage, vk::AccessFlags srcAccess,
        vk::PipelineStageFlags dstStage, vk::AccessFlags dstAccess,
        vk::ImageAspectFlags aspect);

		const Device& device_;
		//const RenderPass& renderPass_;
		Window& window_;
		const CommandPool& commandPool_;
		std::optional<Swapchain> swapchain_;
		std::unique_ptr<CommandBuffer> commandBuffer_;
		uint32_t minImageCount_;

		uint32_t framesInFlight_;
		uint32_t currentFrameIndex_ = 0;
		uint32_t currentImageIndex_ = 0;
		bool isFrameStarted_ = false;

		
		std::unique_ptr<RenderPass> renderPass_;
		std::vector<IComputeStep*> computeSteps_;
		std::vector<IRenderPass*> passes_;
		std::vector<vk::UniqueFence> inFlightFences_;
		std::vector<vk::UniqueSemaphore> readyForRenderengSemaphores_;
		std::vector<vk::UniqueSemaphore> readyForPresentingSemaphores_;
		std::vector<vk::Fence> imagesInFlightFences_;
	};
}
#endif