#ifndef _DE_RENDER_SYSTEM_
#define _DE_RENDER_SYSTEM_

#include"glm_config.hpp"

#include<vulkan/vulkan.hpp>
#include<memory>
#include<vector>
#include<utility>
#include<array>

#include"DE_Pipeline.hpp"
#include"SSBO/DE_TransformBuffer.hpp"
#include"DE_Shaders.hpp"
#include"DE_PushConstants.hpp"
#include"core/DE_SetSlot.hpp"


namespace de {

	class RenderContext;
	class RenderPass;
	class GameObject;
	class Camera;
	class DescriptorSetLayout;
	class GeometryPool;
	struct FrameInfo;
	class Material;
	class TransformBuffer;
	class IndirectCommandBuffer;


	class RenderSystem {
	public:

		RenderSystem(const RenderContext& renderContext, const RenderPass& renderPass);
		
		void setDefaultMaterial(std::shared_ptr<Material> material) noexcept {
			defaultMaterial_ = std::move(material);
		}
		void setMaterialDescriptorSet(vk::DescriptorSet set) noexcept {
			materialDescriptorSet_ = set;
		}
		void setMaterialBufferDescriptorSet(vk::DescriptorSet set) noexcept {
			materialBufferDescriptorSet_ = set;
		}
		void setTransformBuffer(TransformBuffer& transformBuffer) noexcept {
			transformBuffer_ = &transformBuffer;
		}
		void setIndirectBuffer(IndirectCommandBuffer& indirectBuffer) noexcept {
			indirectBuffer_ = &indirectBuffer;
		}

		void setGeometryPool(const GeometryPool& geometryPool);

		void renderGameObjects(const FrameInfo& frameInfo);
	private:
		void createPipeline(const vk::Extent2D extent, std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
		
		const Device* device_;
		const RenderPass& renderPass_;
		vk::Extent2D extent_;

		inline static constexpr std::array<SetSlot, 4> kRequiredSlots{
			SetSlot::eGlobal, SetSlot::eTextures,
			SetSlot::eMaterialData, SetSlot::eTransform
		};
		static_assert(kRequiredSlots.size() == kSetSlotCount);
		static_assert(std::is_sorted(kRequiredSlots.begin(), kRequiredSlots.end()),
					"layout index must equal to_underlying(SetSlot)");

		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<de::Shader> vertexShader_ = nullptr;
		std::unique_ptr<de::Shader> fragmentShader_ = nullptr;
		std::shared_ptr<Material> defaultMaterial_ = nullptr;

		const GeometryPool* geometryPool_ = nullptr;
		vk::DescriptorSet materialDescriptorSet_;
		vk::DescriptorSet materialBufferDescriptorSet_;

		TransformBuffer* transformBuffer_ = nullptr;
		IndirectCommandBuffer* indirectBuffer_ = nullptr;

		std::vector<GPUTransformData> transforms_;
		std::vector<vk::DrawIndexedIndirectCommand> indirectCommands_;
	};

}
#endif