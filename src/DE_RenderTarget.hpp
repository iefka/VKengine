#ifndef _DE_RENDER_TARGET_
#define _DE_RENDER_TARGET_

#include<vulkan/vulkan.hpp>
#include"DE_Swapchain.hpp"

namespace de {

	class RenderTarget {
	public:

		virtual ~RenderTarget() = default;
		
		virtual vk::Framebuffer getFramebuffer(uint32_t imageIndex) const = 0;
		virtual vk::RenderPass getRenderPass() const = 0;
		virtual vk::Extent2D getExtent() const = 0;
	};

	class SwapchainRenderTarget : public RenderTarget {
	public:

		// swapchain pointer is nullptr on creation, use rebind() befor use!
		SwapchainRenderTarget() = default;
		void rebind(const Swapchain& swapchain) { swapchain_ = &swapchain; }

		 vk::Framebuffer getFramebuffer(uint32_t imageIndex) const override{
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
}

#endif