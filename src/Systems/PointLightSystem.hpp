#ifndef _DE_POINT_LIHGT_SYSTEM_
#define _DE_POINT_LIHGT_SYSTEM_

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
	struct FrameInfo;
	struct GlobalUBO;
	struct RenderContext;
	struct PassContext;

	class PointLightSystem {
	public:

		PointLightSystem(const RenderContext& renderContext,
			const PassContext& passContext);

		void update(GameObject::Map& gameObjects, GlobalUBO& ubo, float deltaTime, float totalTime);
		void render(const FrameInfo& frameInfo);
	private:
		void createPipeline(const vk::Extent2D extent, 
		const std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
	 	const vk::PipelineCache& cache,
		vk::Format colorFormat, vk::Format depthFormat);

		const Device* device_;

		inline static constexpr std::array kSlots{ SetSlot::eGlobal};

		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<de::Shader> vertexShader_ = nullptr;
		std::unique_ptr<de::Shader> fragmentShader_ = nullptr;

	};

}
#endif