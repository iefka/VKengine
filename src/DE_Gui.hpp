//#include<memory>
//#include"third_party/imgui/imgui.h"
//
//
//namespace de {
//	class Texture;
//	class Device;
//	class RenderPass;
//	class Swapchain;
//	struct FrameInfo;
//
//	class GUI {
//	public:
//		GUI(const Device& device);
//		~GUI();
//		void renderGUI(FrameInfo& frameInfo);
//
//	private:
//
//		std::unique_ptr<Texture> fontTexture_;
//		std::unique_ptr<ImDrawData> drawData_;
//
//		vk::Extent2D currentExtent_;
//	};
//	
//}