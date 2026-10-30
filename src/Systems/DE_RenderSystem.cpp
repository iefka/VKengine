#include"DE_RenderSystem.hpp"
#include"DE_GeometryPool.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Device.hpp"
#include"DE_Camera.hpp"
#include"DE_GameObject.hpp"
#include"DE_RenderPass.hpp"
#include"DE_FrameData.hpp"
#include"Material.hpp"
#include"SSBO/DE_TransformBuffer.hpp"
#include"DE_IndirectCommandBuffer.hpp"
#include"ranges"

namespace de {

	RenderSystem::RenderSystem(const Device& device,
		const RenderPass& renderPass,
		const vk::Extent2D extent,
		std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts) :
		device_{device},
		renderPass_{renderPass},
		extent_{extent}{

		vertexShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/vertex.vert.spv");
		fragmentShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/fragment.frag.spv");

		createPipeline(extent, descriptorSetLayouts);

	}

	void RenderSystem::setGeometryPool(const GeometryPool& geometryPool){
		geometryPool_ = &geometryPool;
	}

	void RenderSystem::renderGameObjects(const FrameInfo& frameInfo){

		if (materialDescriptorSet_) {
			frameInfo.commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*graphicsPipeline_->getLayout(), 1, materialDescriptorSet_, {}
			);
		}

		if (materialBufferDescriptorSet_) {
			frameInfo.commandBuffer.bindDescriptorSets(
				vk::PipelineBindPoint::eGraphics,
				*graphicsPipeline_->getLayout(), 2, materialBufferDescriptorSet_, {}
			);
		}

		frameInfo.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getLayout(), 0, frameInfo.descriptorSet, {});

		frameInfo.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getPipeline());

		if (geometryPool_) {
			geometryPool_->bind(frameInfo.commandBuffer);
		}

		std::vector<GPUTransformData> transforms;
		std::vector<vk::DrawIndexedIndirectCommand> indirectCommands;
		uint32_t instanceIndex = 0;
		for (auto& kv : frameInfo.gameObjects) {

			auto& object = kv.second;
			if (!object.mesh) continue;

			const auto normalMatrix = object.transform.normalMatrix();
			const auto modelMatrix = object.transform.mat4();
			const auto& subMeshes = object.mesh->getSubMeshes();
			
			for (size_t i = 0; i < subMeshes.size(); ++i){
				const Material* material = (i < object.materialsByMesh_.size() && object.materialsByMesh_.at(i)) ?
					object.materialsByMesh_.at(i).get() : defaultMaterial_.get();

				transforms.push_back(GPUTransformData{
					.modelMatrix = modelMatrix,
					.normalMatrix = normalMatrix,
					.materialIndex = material->getMaterialIndex()
				});
				indirectCommands.push_back(object.mesh->getIndirectCommand(
					static_cast<uint32_t>(i), instanceIndex)
				);
				++instanceIndex;
			}
		}

			if (transformBuffer_) {
				transformBuffer_->upload(frameInfo.frameIndex, transforms);
				frameInfo.commandBuffer.bindDescriptorSets(
					vk::PipelineBindPoint::eGraphics,
					*graphicsPipeline_->getLayout(), 3,
					transformBuffer_->getDescriptorSet(frameInfo.frameIndex), {}
				);
			}

			if (indirectBuffer_ && !indirectCommands.empty()) {
				indirectBuffer_->upload(frameInfo.frameIndex, indirectCommands);
				
				frameInfo.commandBuffer.drawIndexedIndirect(
					indirectBuffer_->getBuffer(frameInfo.frameIndex),
					0,
					static_cast<uint32_t>(indirectCommands.size()),
					sizeof(vk::DrawIndexedIndirectCommand)
				);
			}
		
	}

	void RenderSystem::createPipeline(const vk::Extent2D extent, std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts) {

		const auto& vertexAttributes = de::Mesh::Vertex::getAttributes();
		const auto& bindingDescription = de::Mesh::Vertex::getBindings();

		auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{}
			.setVertexAttributeDescriptions(vertexAttributes)
			.setVertexBindingDescriptions(bindingDescription);

		const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo{}
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setCullMode(vk::CullModeFlagBits::eBack)
			.setFrontFace(vk::FrontFace::eCounterClockwise)
			.setLineWidth(1.f);

		auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{}
			.setBlendEnable(false)
			.setColorWriteMask(vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA);
		auto colorBlendState = vk::PipelineColorBlendStateCreateInfo{}
		.setAttachments(colorBlendAttachment);

		const auto depthStencil = vk::PipelineDepthStencilStateCreateInfo{}
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLess)
			.setDepthBoundsTestEnable(false);

		auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo{}
		.setRasterizationSamples(vk::SampleCountFlagBits::e1);


		std::vector<vk::DynamicState> dynamicStates{                                         
			vk::DynamicState::eViewport,
			vk::DynamicState::eScissor
		};

		auto builder = de::GraphicsBuilder()
			.addFragmentShader("main", fragmentShader_->getShaderModule())
			.addVertexShader("main", vertexShader_->getShaderModule())
			.setRenderPass(*renderPass_.getRenderPass())
			.setInputAssemblyState(vk::PrimitiveTopology::eTriangleList)
			.setViewportState(extent)
			.setRasterizationState(rasterizationState)
			.setMultisampleState(multisampleInfo)
			.setDynamicStates(dynamicStates)
			.setDsLayouts(descriptorSetLayouts)
			.setVertexInputState(vertexInputState)
			.setColorBlendState(colorBlendState)
			.setDepthStencilState(depthStencil);

		graphicsPipeline_ = std::make_unique<de::GraphicsPipeline>(device_, builder);
	}
}
