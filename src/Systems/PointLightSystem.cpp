#include"PointLightSystem.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Device.hpp"
#include"DE_Camera.hpp"
#include"DE_RenderPass.hpp"
#include"DE_FrameData.hpp"


namespace de {

	PointLightSystem::PointLightSystem(const Device& device,
		const RenderPass& renderPass,
		const vk::Extent2D extent,
		std::vector<DescriptorSetLayout>& descriptorSetLayouts) :
		device_{ device },
		renderPass_{ renderPass },
		extent_{ extent } 
	{
		vertexShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/point_light.vert.spv");
		fragmentShader_ = std::make_unique<de::Shader>(device_, "Shaders/bin/point_light.frag.spv");

		std::vector<vk::DescriptorSetLayout> vkDescriptors(descriptorSetLayouts.size());
		for (size_t i = 0; i < descriptorSetLayouts.size(); i++) {
			vkDescriptors[i] = descriptorSetLayouts[i].getLayout();
		}

		createPipeline(extent, vkDescriptors);
	}

	void PointLightSystem::update(GameObject::Map& gameObjects, GlobalUBO& ubo, float deltaTime,float totalTime){
		int lightIndex{ 0 };

		auto rotateLight = glm::rotate(glm::mat4(1.f), 0.5f * deltaTime, { 0.f, 1.f, 0.f });

		for (auto& kv : gameObjects) {
			auto& object = kv.second;
			if (!object.pointLight_) continue;


			object.transform.translation = glm::vec3(rotateLight * glm::vec4(object.transform.translation, 1.f));

			ubo.pointLights[lightIndex].lightPosition = glm::vec4(object.transform.translation,1.f);
			ubo.pointLights[lightIndex].lightColor = glm::vec4(object.color, object.pointLight_->lightIntensity);
			ubo.pointLights[lightIndex].radius = object.transform.scale.x;

			lightIndex += 1;
		}
		ubo.numLights = lightIndex;
	}

	void PointLightSystem::render(const FrameInfo& frameInfo) {

		frameInfo.commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getLayout(), 0, frameInfo.descriptorSet, {});

		frameInfo.commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline_->getPipeline());

		int lightCount{ 0 };

		for (auto& kv : frameInfo.gameObjects) {
			auto& object = kv.second;
			if (!object.pointLight_) continue;

			lightCount += 1;
		}

		frameInfo.commandBuffer.draw(6 * lightCount, 1, 0, 0);
	}

	void PointLightSystem::createPipeline(const vk::Extent2D extent, std::vector<vk::DescriptorSetLayout>& descriptorSetLayouts) {

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
