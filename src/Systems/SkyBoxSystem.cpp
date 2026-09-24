#include"DE_Descriptors.hpp"
#include"DE_Device.hpp"
#include"DE_RenderPass.hpp"
#include"DE_FrameData.hpp"
#include"Texture.hpp"

#include "SkyBoxSystem.hpp"
#include"ranges"

namespace de {

	SkyBoxSystem::~SkyBoxSystem() = default;
	SkyBoxSystem::SkyBoxSystem(const Device& device,
		const RenderPass& renderPass,
		const vk::Extent2D extent,
		std::vector<DescriptorSetLayout>& descriptorSetLayouts,
		const DescriptorPool& descriptorPool,
		std::string texturePath)
		:renderPass_{renderPass},
		device_{device},
		extent_{ extent } {
	
		sbTexture_ = std::make_unique<Texture>(Texture::loadCubemap(device_,
			texturePath));

		vertexShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/skybox.vert.spv");
		fragmentShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/skybox.frag.spv");



		auto vkDescriptors = 
			descriptorSetLayouts
			| std::views::transform([&](const auto& dsSet) {
				return dsSet.getLayout();
			})
			| std::ranges::to<std::vector>();

		createPipeline(extent_, vkDescriptors);

		auto descriptorInfo = sbTexture_->getDescriptorInfo();
		// later somehow need to rewrite this hardcoded dsLayout[1]
		cubemapDescriptor_ = DescriptorWriter{ device, descriptorPool, descriptorSetLayouts[1] }
			.writeImage(0, &descriptorInfo)
			.build();
	}

	void SkyBoxSystem::render(const FrameInfo& frameInfo){
		frameInfo.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getPipeline());
		frameInfo.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getLayout(),
			0, frameInfo.descriptorSet, {});
		frameInfo.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getLayout(),
			1, cubemapDescriptor_, {});

		frameInfo.commandBuffer.draw(3, 1, 0, 0);
	}

	void SkyBoxSystem::createPipeline(const vk::Extent2D extent,
		std::vector<vk::DescriptorSetLayout>&descriptorSetLayouts){
		
		const auto& vertexAttributes = de::Model::Vertex::getAttributes();
		const auto& bindingDescription = de::Model::Vertex::getBindings();

		auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{}
			.setVertexAttributeDescriptions(vertexAttributes)
			.setVertexBindingDescriptions(bindingDescription);

		const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo{}
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setCullMode(vk::CullModeFlagBits::eNone)
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
			.setDepthWriteEnable(false)
			.setDepthCompareOp(vk::CompareOp::eLessOrEqual)
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
