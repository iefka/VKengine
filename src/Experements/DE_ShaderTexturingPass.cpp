#include "DE_Device.hpp"
#include"DE_Renderer.hpp"
#include"DE_Descriptors.hpp"
#include"DE_RenderPass.hpp"
#include"Utility/DE_Utility.hpp"
#include"DE_Memory.hpp"
#include "DE_Shaders.hpp"
#include"DE_Pipeline.hpp"
#include "DE_ShaderTexturingPass.hpp"


namespace de{
	ShaderTexturingPass::ShaderTexturingPass(const Device& device,
		vk::Extent2D extent,
		const RenderPass& renderPass,
		const Renderer& renderer,
		const std::filesystem::path& computeShaderPath)
		:device_{ device },
		extent_{extent}
		,renderPass_{renderPass}
		,renderer_{ renderer } {
	
		constexpr vk::Format imageFormat{ vk::Format::eR8G8B8A8Unorm };

		pushConstantData_.pcTime = 0.f;

		vertexShader_ = std::make_unique<Shader>(device_,
			"Shaders/bin/fullscreen.vert.spv");
		fragmentShader_ = std::make_unique<Shader>(device_,
			"Shaders/bin/texturePresent.frag.spv");
		computeShader_ = std::make_unique<Shader>(device_,
			computeShaderPath);

		auto imageInfo = ImageUsageInfo{
			0,0,
			vk::ImageCreateInfo{}
				.setImageType(vk::ImageType::e2D)
				.setExtent(vk::Extent3D{extent,1})
				.setMipLevels(1)
				.setArrayLayers(1)
				.setTiling(vk::ImageTiling::eOptimal)
				.setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled)
				.setFormat(imageFormat)
				.setInitialLayout(vk::ImageLayout::eUndefined)
				.setSamples(vk::SampleCountFlagBits::e1)
				.setSharingMode(vk::SharingMode::eExclusive),
			vk::MemoryPropertyFlagBits::eDeviceLocal
		};

		computeImage_ = std::make_unique<Image>(device_, imageInfo);
		device_.transitionImageLayout(computeImage_->getImage(),
			imageFormat, vk::ImageLayout::eUndefined,
			vk::ImageLayout::eGeneral);
		computeImageView_ = utl::createImageView(device_.getLogicalDevice(), computeImage_->getImage(),
			imageFormat, vk::ImageAspectFlagBits::eColor);

		computeImageSampler_ = utl::createTextureSampler(device_.getLogicalDevice(),
			vk::SamplerAddressMode::eClampToEdge, false);

		descriptorPool_ = std::make_unique<DescriptorPool>(
			DescriptorPool::Builder{ device_ }
			.setMaxSets(2)
			.addPoolSize(vk::DescriptorType::eStorageImage, 1)
			.addPoolSize(vk::DescriptorType::eCombinedImageSampler, 1)
			.build()
		);

		computeDescriptorLayout_ = std::make_unique<DescriptorSetLayout>(
			DescriptorSetLayout::Builder{ device }
			.addBinding(0, vk::DescriptorType::eStorageImage
				, vk::ShaderStageFlagBits::eCompute)
			.build()
		);
		graphicsDescriptorLayout_ = std::make_unique<DescriptorSetLayout>(
			DescriptorSetLayout::Builder{ device }
			.addBinding(0, vk::DescriptorType::eCombinedImageSampler
				, vk::ShaderStageFlagBits::eFragment)
			.build()
		);

		auto descriptorImageInfo = vk::DescriptorImageInfo{}
			.setImageLayout(vk::ImageLayout::eGeneral)
			.setImageView(*computeImageView_)
			.setSampler(*computeImageSampler_);

		computeDescriptor_ = DescriptorWriter{ device,*descriptorPool_, *computeDescriptorLayout_ }
			.writeImage(0, &descriptorImageInfo)
			.build();

		graphicsDescriptor_ = DescriptorWriter{ device,*descriptorPool_, *graphicsDescriptorLayout_ }
			.writeImage(0, &descriptorImageInfo)
			.build();


		createPipelines(extent);
	}
	ShaderTexturingPass::~ShaderTexturingPass() = default;

	void ShaderTexturingPass::updateTime(float deltaTime){
	
		pushConstantData_.pcTime = deltaTime;
	}
	std::vector<vk::ClearValue> ShaderTexturingPass::getClearValues() const{
		return std::vector<vk::ClearValue>{
			vk::ClearValue{}.setColor(std::array<float, 4>{{1.0f, 0.0f, 1.0f, 1.0f}}),
			vk::ClearValue{}.setDepthStencil(vk::ClearDepthStencilValue{ 1.0f, 0 }) };
	}
	const RenderTarget& ShaderTexturingPass::getRenderTarget() const{
		return renderer_.getSwapchainRenderTarget();
	}
	void ShaderTexturingPass::preRecord(const FrameData& frameData){
		pushConstant_.setValue(pushConstantData_);

		frameData.commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute,
			*computePipeline_->getPipeline());
		frameData.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
			*computePipeline_->getLayout(), 0, computeDescriptor_, {});
		pushConstant_.push(frameData.commandBuffer, *computePipeline_->getLayout());

		uint32_t groupX = (extent_.width + 15) / 16;
		uint32_t groupY = (extent_.height + 15) / 16;
		frameData.commandBuffer.dispatch(groupX, groupY, 1);

		auto barrier = vk::ImageMemoryBarrier{}
			.setOldLayout(vk::ImageLayout::eGeneral)
			.setNewLayout(vk::ImageLayout::eGeneral)
			.setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
			.setDstAccessMask(vk::AccessFlagBits::eShaderRead)
			.setImage(computeImage_->getImage())
			.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

		frameData.commandBuffer.pipelineBarrier(
			vk::PipelineStageFlagBits::eComputeShader,
			vk::PipelineStageFlagBits::eFragmentShader,
			{}, {}, {}, barrier);

	}
	void ShaderTexturingPass::record(const FrameData & frameData){
	

		frameData.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics,
			*graphicsPipeline_->getPipeline());
		frameData.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
			*graphicsPipeline_->getLayout(), 0, graphicsDescriptor_, {});

		frameData.commandBuffer.draw(3, 1, 0, 0);
	}
	void ShaderTexturingPass::onSwapchainRecreation(const Swapchain & sc){
		return;
	}
	void ShaderTexturingPass::createPipelines(vk::Extent2D extent) {


		//computePipeline
		auto computeBuilder = ComputePipeline::Builer{}
			.setShaderStage(
				vk::PipelineShaderStageCreateInfo{}
				.setModule(computeShader_->getShaderModule())
				.setPName("main")
				.setStage(vk::ShaderStageFlagBits::eCompute)
			)
			.addDsLayout(computeDescriptorLayout_->getLayout())
			.setPushConstantRanges({ pushConstant_.getRange() });
		
			computePipeline_ = std::make_unique<ComputePipeline>(device_, computeBuilder);

		//graphics pipeline
		auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{};

		const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo{}
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setCullMode(vk::CullModeFlagBits::eFront)
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

		auto graphicsBuilder = de::GraphicsBuilder()
			.addFragmentShader("main", fragmentShader_->getShaderModule())
			.addVertexShader("main", vertexShader_->getShaderModule())
			.setRenderPass(*renderPass_.getRenderPass())
			.setInputAssemblyState(vk::PrimitiveTopology::eTriangleList)
			.setViewportState(extent)
			.setRasterizationState(rasterizationState)
			.setMultisampleState(multisampleInfo)
			.setDynamicStates(dynamicStates)
			.setDsLayouts({ graphicsDescriptorLayout_->getLayout() })
			.setVertexInputState(vertexInputState)
			.setColorBlendState(colorBlendState)
			.setDepthStencilState(depthStencil);

		graphicsPipeline_ = std::make_unique<de::GraphicsPipeline>(device_, graphicsBuilder);

	}
}