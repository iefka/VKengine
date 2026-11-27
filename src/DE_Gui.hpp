#include"DE_IRenderPass.hpp"

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
	struct RenderContext;

	class GUIRenderPass : public IRenderPass {
	public:

		 GUIRenderPass(
			 const Instance& instance,
			 const RenderContext& renderContext,
			 Window& window,
			 const Renderer& renderer);
		 ~GUIRenderPass();
		 void record(const FrameData& frameData) override;

	private:
		const Renderer& renderer_;
		std::unique_ptr<DescriptorPool> imguiPool_;
	};
	
	
}