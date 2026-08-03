#include "DE_RenderSystem.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Device.hpp"
#include"DE_Camera.hpp"
#include"DE_GameObject.hpp"
#include"DE_RenderPass.hpp"


namespace de {

	RenderSystem::RenderSystem(const Device& device,
		const RenderPass& renderPass,
		const vk::Extent2D extent,
		std::vector<DescriptorSetLayout>& descriptorSetLayouts) :
		device_{device},
		renderPass_{renderPass},
		extent_{extent},
		pushConstant_{ 
			PushConstant<SimplePushConstant>(vk::ShaderStageFlagBits::eVertex)}
	{
		vertexShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/vertex.spv");
		fragmentShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/fragment.spv");
		
		std::vector<vk::DescriptorSetLayout> vkDescriptors(descriptorSetLayouts.size());
			for (size_t i = 0; i < descriptorSetLayouts.size(); i++){
				vkDescriptors[i] = descriptorSetLayouts[i].getLayout();
			}

		createPipeline(extent, vkDescriptors);
	}

	void RenderSystem::renderGameObjects(const vk::CommandBuffer& commandBuffer,
		const std::vector<GameObject>& gameObjects,
		const Camera& camera,
		const vk::DescriptorSet& descriptorSet){

		//extent_ = extent;

		//auto aspectRaito = static_cast<float>(extent_.width) / static_cast<float>(extent_.height);
		//auto projectionView = camera.getProjectionMatrix(aspectRaito) * camera.getViewMatrix();

		commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getLayout(), 0, descriptorSet, {});

		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getPipeline());

		for (auto& obj : gameObjects) {
			pushData.normalMatrix = obj.transform.normalMatrix();
			pushData.modelMatrix = obj.transform.mat4();
			
			pushConstant_.setValue(pushData);
			pushConstant_.push(commandBuffer, *graphicsPipeline_->getLayout());
			obj.model->bind(commandBuffer);
			obj.model->draw(commandBuffer);
		}
	}

	void RenderSystem::createPipeline(const vk::Extent2D extent, std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts) {

		const auto& vertexAttributes = de::Model::Vertex::getAttributes();
		const auto& bindingDescription = de::Model::Vertex::getBindings();

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
			.setDepthBoundsTestEnable(false)
			.setStencilTestEnable(true);

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
			.addPushConstantRange(pushConstant_.getRange())
			.setColorBlendState(colorBlendState)
			.setDepthStencilState(depthStencil);

		graphicsPipeline_ = std::make_unique<de::GraphicsPipeline>(device_, builder);
	}
}
