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
	class GeometryPool;
	struct FrameInfo;
	class Material;
	class TransformBuffer;
	class IndirectCommandBuffer;


	class RenderSystem {
	public:

		RenderSystem(const Device& device,
			const RenderPass& renderPass,
			 const vk::Extent2D extent,
			std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts);
		
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
		
		const Device& device_;
		const RenderPass& renderPass_;
		vk::Extent2D extent_;


		std::unique_ptr<GraphicsPipeline> graphicsPipeline_ = nullptr;
		std::unique_ptr<de::Shader> vertexShader_ = nullptr;
		std::unique_ptr<de::Shader> fragmentShader_ = nullptr;
		std::shared_ptr<Material> defaultMaterial_ = nullptr;

		const GeometryPool* geometryPool_ = nullptr;
		vk::DescriptorSet materialDescriptorSet_;
		vk::DescriptorSet materialBufferDescriptorSet_;

		TransformBuffer* transformBuffer_ = nullptr;
		IndirectCommandBuffer* indirectBuffer_ = nullptr;
	};

}
#endif