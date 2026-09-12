#include<memory>
#include"third_party/imgui/imgui.h"


namespace de {
	class Texture;
	class Device;
	class RenderPass;
	class Swapchain;
	struct FrameInfo;

	class GUI {
	public:
		GUI(const Device& device,const Swapchain& swapchain);
		~GUI();
		void renderGUI(FrameInfo& frameInfo);

		vk::Framebuffer getFrameBuffer(uint32_t imageIndex) const{}
		vk::RenderPass getRenderPass() const {}
		vk::Extent2D getSwapchainExtent() const {}

	private:

		RenderPass createRenderPass(const Device& device);

		std::unique_ptr<Texture> fontTexture_;
		std::unique_ptr<ImDrawData> drawData_;

		const Swapchain& swapchain_;
		vk::Extent2D currentExtent_;
		RenderPass renderPass_;
	};
	
}