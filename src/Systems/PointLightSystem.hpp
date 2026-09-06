#ifndef _DE_POINT_LIHGT_SYSTEM_
#define _DE_POINT_LIHGT_SYSTEM_

#include"glm_config.hpp"

#include<vulkan/vulkan.hpp>
#include<memory>
#include<vector>

#include"DE_Pipeline.hpp"
#include"DE_Shaders.hpp"
#include"DE_GameObject.hpp"


namespace de {

	class Device;
	class RenderPass;
	class Camera;
	class DescriptorSetLayout;
	struct FrameInfo;
	struct GlobalUBO;


	class PointLightSystem {
	public:

		PointLightSystem(const Device& device,
			const RenderPass& renderPass,
			const vk::Extent2D extent,
			std::vector<DescriptorSetLayout>& descriptorSetLayouts);

		void update(GameObject::Map& gameObjects, GlobalUBO& ubo, float deltaTime, float totalTime);
		void render(const FrameInfo& frameInfo);
	private:
		void createPipeline(const vk::Extent2D extent, std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);

		const Device& device_;
		const RenderPass& renderPass_;
		vk::Extent2D extent_;


		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<de::Shader> vertexShader_ = nullptr;
		std::unique_ptr<de::Shader> fragmentShader_ = nullptr;

	};

}
#endif