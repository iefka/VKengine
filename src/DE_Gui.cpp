#include "DE_Gui.hpp"
#include"DE_Instance.hpp"
#include"DE_Device.hpp"
#include"DE_Window.hpp"
#include"DE_Swapchain.hpp"
#include"DE_RenderPass.hpp"
#include"DE_Renderer.hpp"
#include"DE_Descriptors.hpp"
#include"core/DE_RenderContext.hpp"

#include"third_party/imgui/imgui.h"
#include"third_party/imgui/backends/imgui_impl_vulkan.h"
#include"third_party/imgui/backends/imgui_impl_glfw.h"

namespace de {



	GUIRenderPass::GUIRenderPass(
		const Instance& instance,
		const RenderContext& renderContext,
		Window& window,
		const Renderer& renderer)
	:renderer_{renderer}{
	

		ImGui::CreateContext();
		IMGUI_CHECKVERSION();

		imguiPool_ = std::make_unique<DescriptorPool>(
			DescriptorPool::Builder{ *renderContext.device }
			.setMaxSets(1000)
			.addPoolSize(vk::DescriptorType::eSampledImage, 500)
			.addPoolSize(vk::DescriptorType::eSampler, 500)

			.setPoolFlags(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet)
			.build()
		);

		VkFormat colorFormat = static_cast<VkFormat>(window.getSurfaceFormat().format);

		ImGui_ImplVulkan_InitInfo initInfo{
			.ApiVersion = vk::makeApiVersion(0,1,4,0),
			.Instance = instance.getInstance(),
			.PhysicalDevice = renderContext.device->getPhysicalDevice(),
			.Device = renderContext.device->getLogicalDevice(),
			.QueueFamily = renderContext.device->getGraphicsQueue().queuFamilyIndex,
			.Queue = renderContext.device->getGraphicsQueue().queue,
			.DescriptorPool = imguiPool_->getPool(),
			.DescriptorPoolSize = 0,               
			.MinImageCount = renderer_.getSwapchain().getMinImageCount(),
			.ImageCount = renderer_.getSwapchain().getImageCount(),
			.PipelineCache = renderContext.pipelinesCache,
			.PipelineInfoMain = {
				.RenderPass = nullptr,
				.Subpass = 0,
				.MSAASamples = VK_SAMPLE_COUNT_1_BIT,
				.ExtraDynamicStates = {},
				.PipelineRenderingCreateInfo = 
					VkPipelineRenderingCreateInfo{
						.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO,
						.pNext = nullptr,
						.viewMask = 0,
						.colorAttachmentCount = 1,
						.pColorAttachmentFormats = &colorFormat,
						.depthAttachmentFormat = VK_FORMAT_UNDEFINED,
						.stencilAttachmentFormat = VK_FORMAT_UNDEFINED
					}
			},
			.UseDynamicRendering = true,
			.Allocator = nullptr,
			.CheckVkResultFn = nullptr,
			.MinAllocationSize = 0,
			.CustomShaderVertCreateInfo = {},
			.CustomShaderFragCreateInfo = {},
		};
		

		ImGui_ImplGlfw_InitForVulkan(window.getGlfwWindow(), true);
		ImGui_ImplVulkan_Init(&initInfo);

	}

	GUIRenderPass::~GUIRenderPass() {
		ImGui_ImplVulkan_Shutdown();
		ImGui_ImplGlfw_Shutdown();
		ImGui::DestroyContext();
	}

	void GUIRenderPass::record(const FrameData& frameData){
		ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), frameData.commandBuffer);

	}




}
