#ifndef _DE_RENDER_SYSTEM_
#define _DE_RENDER_SYSTEM_

#include"glm_config.hpp"

#include<vulkan/vulkan.hpp>
#include<memory>
#include<vector>

#include"DE_Pipeline.hpp"
#include"DE_Shaders.hpp"
#include"DE_PushConstants.hpp"

namespace de {

	class Device;
	class RenderPass;
	class GameObject;
	class Camera;
	class DescriptorSetLayout;
	struct FrameInfo;

	struct SimplePushConstant {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{};
	};

	class RenderSystem {
	public:

		RenderSystem(const Device& device,
			const RenderPass& renderPass,
			 const vk::Extent2D extent,
			std::vector<DescriptorSetLayout>& descriptorSetLayouts);

		void renderGameObjects(const FrameInfo& frameInfo);
	private:
		void createPipeline(const vk::Extent2D extent, std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
		
		const Device& device_;
		const RenderPass& renderPass_;
		vk::Extent2D extent_;


		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<de::Shader> vertexShader_ = nullptr;
		std::unique_ptr<de::Shader> fragmentShader_ = nullptr;

		PushConstant<SimplePushConstant> pushConstant_;
		SimplePushConstant pushData{};
	};

}
#endif