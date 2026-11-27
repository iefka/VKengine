#include "SkyBoxSystem.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Device.hpp"
#include"DE_RenderPass.hpp"
#include"DE_FrameData.hpp"
#include"Texture.hpp"
#include"core/DE_RenderContext.hpp"
#include"core/DE_PassContext.hpp"

#include"ranges"

namespace de {

	SkyBoxSystem::~SkyBoxSystem() = default;
	SkyBoxSystem::SkyBoxSystem(const RenderContext& renderContext,
		const PassContext& passContext,
		const DescriptorPool& descriptorPool,
		std::string texturePath)
		:device_{renderContext.device}{
	
		sbTexture_ = std::make_unique<Texture>(Texture::loadCubemap(*device_,
			texturePath));

		vertexShader_ = std::make_unique<de::Shader>(*device_, "Shaders/bin/skybox.vert.spv");
		fragmentShader_ = std::make_unique<de::Shader>(*device_, "Shaders/bin/skybox.frag.spv");


		auto descriptorInfo = sbTexture_->getDescriptorInfo();

		cubemapLayout_ = std::make_unique<DescriptorSetLayout>(
			DescriptorSetLayout::Builder{ *device_ }
			.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
			.build()
		);

		cubemapDescriptor_ = DescriptorWriter{ *device_, descriptorPool, *cubemapLayout_}
			.writeImage(0, &descriptorInfo)
			.build();

		const std::array<vk::DescriptorSetLayout, 2> layouts{
    		renderContext.descriptorLayouts->get(SetSlot::eGlobal),
    		cubemapLayout_->getLayout()
		};

		createPipeline(passContext.extent, layouts, renderContext.pipelinesCache,
		passContext.colorFormat, passContext.depthFormat);

	}

	void SkyBoxSystem::render(const FrameInfo& frameInfo){

		std::array<vk::DescriptorSet, kSlots.size()> sets{};
		sets[std::to_underlying(SkyBoxSlots::eGlobal)] = frameInfo.descriptorSet;
		sets[std::to_underlying(SkyBoxSlots::eSkyBox)] = cubemapDescriptor_;
		bindSets(frameInfo.commandBuffer, *graphicsPipeline_->getLayout(), sets);

		frameInfo.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getPipeline());

		frameInfo.commandBuffer.draw(3, 1, 0, 0);
	}

	void SkyBoxSystem::createPipeline(const vk::Extent2D extent, 
		const std::span<const vk::DescriptorSetLayout> descriptorSetLayouts,
	 	const vk::PipelineCache& cache,
		vk::Format colorFormat, vk::Format depthFormat){

		auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{};

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
