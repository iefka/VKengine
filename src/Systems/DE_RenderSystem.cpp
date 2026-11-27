#include"DE_RenderSystem.hpp"
#include"core/DE_RenderContext.hpp"
#include"core/DE_PassContext.hpp"
#include"core/DE_Assert.hpp"
#include"DE_GeometryPool.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Device.hpp"
#include"DE_Camera.hpp"
#include"DE_GameObject.hpp"
#include"DE_FrameData.hpp"
#include"Material.hpp"
#include"SSBO/DE_TransformBuffer.hpp"
#include"DE_IndirectCommandBuffer.hpp"
#include<ranges>

#include<vector>

namespace de {

	RenderSystem::RenderSystem (const RenderContext& renderContext, const PassContext& passContext) 
		:device_{renderContext.device}{

		vertexShader_ = std::make_unique<de::Shader>(*device_, "Shaders/bin/vertex.vert.spv");
		fragmentShader_ = std::make_unique<de::Shader>(*device_, "Shaders/bin/fragment.frag.spv");


		auto layouts = kSlots
		|std::views::transform([&](auto slot){
			return renderContext.descriptorLayouts->get(slot);
		})
		|std::ranges::to<std::vector>();

		createPipeline(passContext.extent, layouts, renderContext.pipelinesCache,
		passContext.colorFormat, passContext.depthFormat);

	}

	void RenderSystem::setGeometryPool(const GeometryPool& geometryPool){
		geometryPool_ = &geometryPool;
	}

	void RenderSystem::renderGameObjects(const FrameInfo& frameInfo){
		DE_ASSERT(transformBuffer_ && indirectBuffer_ && defaultMaterial_);
		const auto pipelineLayout = *graphicsPipeline_->getLayout();
		const auto pipeline = *graphicsPipeline_->getPipeline();
		const auto cmd = frameInfo.commandBuffer;
		transforms_.clear();
		indirectCommands_.clear();

		cmd.bindPipeline(vk::PipelineBindPoint::eGraphics,pipeline);

		if (geometryPool_) {
			geometryPool_->bind(frameInfo.commandBuffer);
		}

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

				transforms_.push_back(GPUTransformData{
					.modelMatrix = modelMatrix,
					.normalMatrix = normalMatrix,
					.materialIndex = material->getMaterialIndex(frameInfo.frameIndex)
				});
				indirectCommands_.push_back(object.mesh->getIndirectCommand(
					static_cast<uint32_t>(i), instanceIndex)
				);
				++instanceIndex;
			}
		}

	
		transformBuffer_->upload(frameInfo.frameIndex, transforms_);
		indirectBuffer_->upload(frameInfo.frameIndex, indirectCommands_);
		

		std::array<vk::DescriptorSet, kSlots.size()> sets{};
		sets[std::to_underlying(SetSlot::eGlobal)] = frameInfo.descriptorSet;
		sets[std::to_underlying(SetSlot::eTextures)] = materialDescriptorSet_;
		sets[std::to_underlying(SetSlot::eMaterialData)] = materialBufferDescriptorSet_;
		sets[std::to_underlying(SetSlot::eTransforms)] =  
			transformBuffer_->getDescriptorSet(frameInfo.frameIndex);
		
		for (uint32_t i = 0; i < kSlots.size(); ++i) {
			DE_ASSERT(sets[i]);
		}

		bindSets(cmd, pipelineLayout, sets);

		if (!indirectCommands_.empty()) {
				
			frameInfo.commandBuffer.drawIndexedIndirect(
				indirectBuffer_->getBuffer(frameInfo.frameIndex),
				0,
				static_cast<uint32_t>(indirectCommands_.size()),
				sizeof(vk::DrawIndexedIndirectCommand)
			);
		}

	}

	void RenderSystem::createPipeline(const vk::Extent2D extent, 
		const std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
	 	const	vk::PipelineCache& cache,
		vk::Format colorFormat, vk::Format depthFormat) {

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
			.setRenderingFormat(colorFormat, depthFormat)
			.setInputAssemblyState(vk::PrimitiveTopology::eTriangleList)
			.setViewportState(extent)
			.setRasterizationState(rasterizationState)
			.setMultisampleState(multisampleInfo)
			.setDynamicStates(dynamicStates)
			.setPipelineCache(cache)
			.setDsLayouts(descriptorSetLayouts)
			.setVertexInputState(vertexInputState)
			.setColorBlendState(colorBlendState)
			.setDepthStencilState(depthStencil);

		graphicsPipeline_ = std::make_unique<de::GraphicsPipeline>(*device_, builder);
	}
}
