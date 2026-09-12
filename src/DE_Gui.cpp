#include"DE_Device.hpp"
#include"Texture.hpp"
#include"DE_RenderPass.hpp"
#include"DE_Swapchain.hpp"
#include "DE_Gui.hpp"

namespace de {

	 Texture static createFontTexture(const Device& device, ImGuiIO& io, std::string fontPath) {


		auto fontCfg = ImFontConfig{};
		fontCfg.FontDataOwnedByAtlas = false;
		fontCfg.RasterizerMultiply = 1.5f;
		fontCfg.SizePixels = 600.f / 32.f;
		fontCfg.PixelSnapH = true;
		fontCfg.OversampleH = 4;
		fontCfg.OversampleV = 4;

		ImFont* font = io.Fonts->AddFontFromFileTTF(fontPath.c_str(),
			fontCfg.SizePixels, &fontCfg);

		unsigned char* pixels {nullptr};
		int width, height;


		//32 byte for pixel
		io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

		if (!pixels) {
			throw std::runtime_error("Font data load failed");
		}

		io.Fonts->TexID = ImTextureID(0);
		io.FontDefault = font;
		io.DisplayFramebufferScale = ImVec2(1, 1);

		return Texture(device, vk::Format::eR8G8B8A8Unorm,
			pixels, width, height);
	}

	 
	 RenderPass GUI::createRenderPass(const Device& device)
	 {
		 auto renderPass =  RenderPass::Builder{device}
			 .setColorAttachment(,)
	 }

	GUI::GUI(const Device& device, const Swapchain& swapchain) :swapchain_{swapchain} {
		ImGuiIO& io = ImGui::GetIO();

		fontTexture_ = std::make_unique<Texture>(
			createFontTexture(device,io,"Fonts/OpenSans.ttf")
		);
	}

	GUI::~GUI() = default;

	void GUI::renderGUI(FrameInfo& frameInfo){
	
		
	}
	
}
