#pragma once

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
	class DescriptorPool;
	struct FrameInfo;
	struct GlobalUBO;
	class Texture;


	class SkyBoxSystem {
	public:

		SkyBoxSystem(const Device& device,
			const RenderPass& renderPass,
			const vk::Extent2D extent,
			std::vector<DescriptorSetLayout>& descriptorSetLayouts,
			const DescriptorPool& descriptorPool,
			std::string texturePath);

		~SkyBoxSystem();

		void render(const FrameInfo& frameInfo);
	private:
		void createPipeline(const vk::Extent2D extent, std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);

		const Device& device_;
		const RenderPass& renderPass_;
		vk::Extent2D extent_;

		vk::DescriptorSet cubemapDescriptor_;
		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<Shader> vertexShader_ = nullptr;
		std::unique_ptr<Shader> fragmentShader_ = nullptr;
		std::unique_ptr<Texture> sbTexture_ = nullptr;

	};

}
