#include"DE_IRenderPass.hpp"
#include"DE_RenderTarget.hpp"


#include<memory>


namespace de {

	class Instance;
	class Device;
	struct FrameData;
	class Renderer;
	class DescriptorPool;
	class RenderPass;
	class Window;
	class GUIRenderPass;
	
	
	class GUIRenderTarget : public RenderTarget {
	public:

		// swapchain pointer is nullptr on creation, use rebind() befor use!
		GUIRenderTarget() = default;
		void rebind(const Swapchain& swapchain, GUIRenderPass& guiPass) {
			swapchain_ = &swapchain;
			guiPass_ = &guiPass;
		}

		vk::Framebuffer getFramebuffer(uint32_t imageIndex) const override;
		vk::RenderPass getRenderPass() const override;
		vk::Extent2D getExtent() const override;
	private:
		const Swapchain* swapchain_ = nullptr;
		GUIRenderPass* guiPass_ = nullptr;
	};

	class GUIRenderPass : public IRenderPass {
	public:

		 GUIRenderPass(
			 const Instance& instance,
			 const Device& device,
			 Window& window,
			 const Renderer& renderer);
		 ~GUIRenderPass();
		 std::vector<vk::ClearValue> getClearValues()  const override;
		 const RenderTarget& getRenderTarget() const override;
		 void record(const FrameData& frameData) override;
		 void onSwapchainRecreation(const Swapchain& sc) override;

		 vk::RenderPass getRenderPass() const noexcept;

	private:
		const Renderer& renderer_;
		std::unique_ptr<DescriptorPool> imguiPool_;
		std::unique_ptr<RenderPass> renderPass_;
		GUIRenderTarget renderTarget_;
	};
	
	
}