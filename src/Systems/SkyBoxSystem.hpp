#pragma once

#include"glm_config.hpp"

#include<vulkan/vulkan.hpp>
#include<memory>
#include<vector>

#include"DE_Pipeline.hpp"
#include"DE_Shaders.hpp"
#include"DE_GameObject.hpp"
#include"core/DE_SetSlot.hpp"
#include"core/DE_SlotUtils.hpp"
#include<utility>
#include <span>


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
			const std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
	 		const vk::PipelineCache& cache,
			vk::Format colorFormat, vk::Format depthFormat);

		const Device* device_;

		enum class SkyBoxSlots  : uint32_t{
			eGlobal = std::to_underlying(SetSlot::eGlobal),
			eSkyBox = std::to_underlying(SetSlot::eGlobal) + 1
		};


		inline static constexpr std::array kSlots{ SkyBoxSlots::eGlobal, SkyBoxSlots::eSkyBox};
		static_assert(areConsecutiveFromZero(kSlots) && "slots must go 0..N");

		
		std::unique_ptr<de::DescriptorSetLayout> cubemapLayout_;
		vk::DescriptorSet cubemapDescriptor_;
		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<Shader> vertexShader_ = nullptr;
		std::unique_ptr<Shader> fragmentShader_ = nullptr;
		std::unique_ptr<Texture> sbTexture_;

	};

}
