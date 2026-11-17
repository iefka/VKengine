#pragma once

#include"glm_config.hpp"

#include<vulkan/vulkan.hpp>
#include<memory>
#include<vector>

#include"DE_Pipeline.hpp"
#include"DE_Shaders.hpp"
#include"DE_GameObject.hpp"
#include"core/DE_SetSlot.hpp"


namespace de {

	class Device;
	class RenderPass;
	class Camera;
	class DescriptorSetLayout;
	class DescriptorPool;
	struct FrameInfo;
	struct GlobalUBO;
	class Texture;
	struct PassContext;
	struct RenderContext;


	class SkyBoxSystem {
	public:

		SkyBoxSystem(const RenderContext& renderContext,
			const PassContext& passContext,
			const DescriptorPool& descriptorPool,
			std::string texturePath);

		~SkyBoxSystem();

		void render(const FrameInfo& frameInfo);
	private:
		void createPipeline(const vk::Extent2D extent, 
			std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts, vk::PipelineCache cache);

		const Device* device_;
		const RenderPass* renderPass_;

		inline static constexpr std::array kSlots{ SetSlot::eGlobal};
		static_assert(de::areConsecutiveFromZero(kSlots), "slots must be 0..N-1 in order");

		std::unique_ptr<de::DescriptorSetLayout> cubemapLayout_;
		vk::DescriptorSet cubemapDescriptor_;
		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<Shader> vertexShader_ = nullptr;
		std::unique_ptr<Shader> fragmentShader_ = nullptr;
		std::unique_ptr<Texture> sbTexture_;

	};

}
