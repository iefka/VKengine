#include"DE_Instance.hpp"
#include"DE_Device.hpp"
#include"DE_Window.hpp"
#include"DE_Swapchain.hpp"
#include"DE_RenderPass.hpp"
#include"DE_Renderer.hpp"
#include"DE_Descriptors.hpp"
#include "DE_Gui.hpp"


#include"third_party/imgui/imgui.h"
#include"third_party/imgui/backends/imgui_impl_vulkan.h"
#include"third_party/imgui/backends/imgui_impl_glfw.h"

namespace de {

	// Texture static createFontTexture(const Device& device, ImGuiIO& io, std::string fontPath) {


	//	auto fontCfg = ImFontConfig{};
	//	fontCfg.FontDataOwnedByAtlas = false;
	//	fontCfg.RasterizerMultiply = 1.5f;
	//	fontCfg.SizePixels = 600.f / 32.f;
	//	fontCfg.PixelSnapH = true;
	//	fontCfg.OversampleH = 4;
	//	fontCfg.OversampleV = 4;

	//	ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(),
	//		fontCfg.SizePixels, &fontCfg);

	//	unsigned char* pixels {nullptr};
	//	int width, height;


	//	//32 byte for pixel
	//	io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

	//	if (!pixels) {
	//		throw std::runtime_error("Font data load failed");
	//	}

	//	io.Fonts->TexID = ImTextureID(0);
	//	io.FontDefault = font;
	//	io.DisplayFramebufferScale = ImVec2(1, 1);

	//	return Texture(device, vk::Format::eR8G8B8A8Unorm,
	//		pixels, width, height);
	//}

	//GUI::GUI(const Device& device) {
	//}

	//GUI::~GUI() = default;

	//void GUI::renderGUI(FrameData& frameInfo){
	//
	//}
	



	vk::Framebuffer GUIRenderTarget::getFramebuffer(uint32_t imageIndex) const{
		return swapchain_->getFrameBuffer(imageIndex);
	}

	vk::RenderPass GUIRenderTarget::getRenderPass() const{
		return guiPass_->getRenderPass();
	}

	vk::Extent2D GUIRenderTarget::getExtent() const{
		return swapchain_->getSwapchainExtent();
	}

	GUIRenderPass::GUIRenderPass(
		const Instance& instance,
		const Device& device,
		Window& window,
		const Renderer& renderer)
	:renderer_{renderer}{
	

		ImGui::CreateContext();
		IMGUI_CHECKVERSION();
		renderPass_ = std::make_unique<RenderPass>(
			RenderPass::Builder{ device }
			.setColorAttachment(
				window.getSurfaceFormat().format,
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eLoad,
				vk::AttachmentStoreOp::eStore,
				vk::ImageLayout::ePresentSrcKHR)
			//time
			.setDepthAttachment(
				vk::Format::eD32Sfloat,            
				vk::SampleCountFlagBits::e1,
				vk::AttachmentLoadOp::eDontCare,
				vk::AttachmentStoreOp::eDontCare,
				vk::ImageLayout::eDepthStencilAttachmentOptimal,
				vk::ImageLayout::eDepthStencilAttachmentOptimal)
			.build()
		);

		renderTarget_.rebind(renderer_.getSwapchain(), *this);

		imguiPool_ = std::make_unique<DescriptorPool>(
			DescriptorPool::Builder{ device }
			.setMaxSets(1000)
			.addPoolSize(vk::DescriptorType::eSampledImage, 500)
			.addPoolSize(vk::DescriptorType::eSampler, 500)

			.setPoolFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.build()
		);



		ImGui_ImplVulkan_InitInfo initInfo{
		 .ApiVersion = vk::makeApiVersion(0,1,4,0),
		 .Instance = instance.getInstance(),
		 .PhysicalDevice = device.getPhysicalDevice(),
		 .Device = device.getLogicalDevice(),
		 .QueueFamily = device.getGraphicsQueue().queuFamilyIndex,
		 .Queue = device.getGraphicsQueue().queue,
		 .DescriptorPool = imguiPool_->getPool(),
		 .MinImageCount = renderer_.getSwapchain().getMinImageCount(),
		 .ImageCount = renderer_.getSwapchain().getImageCount(),
		 .UseDynamicRendering = false
		};
		initInfo.PipelineInfoMain.RenderPass = *renderPass_->getRenderPass();
		initInfo.PipelineInfoMain.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
		

		ImGui_ImplGlfw_InitForVulkan(window.getGlfwWindow(), true);
		ImGui_ImplVulkan_Init(&initInfo);

	}

	GUIRenderPass::~GUIRenderPass() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	std::vector<vk::ClearValue> GUIRenderPass::getClearValues() const{
		
		//dont clear
		return {};
	}

	const RenderTarget& GUIRenderPass::getRenderTarget() const{
		return renderTarget_;
	}

	void GUIRenderPass::record(const FrameData& frameData){
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frameData.commandBuffer);

	}

	void GUIRenderPass::onSwapchainRecreation(const Swapchain& sc){
		renderTarget_.rebind(renderer_.getSwapchain(), *this);
	}

	vk::RenderPass GUIRenderPass::getRenderPass() const noexcept
	{
		return *renderPass_->getRenderPass();
	}

}
