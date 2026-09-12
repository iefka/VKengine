#include"DE_MainRenderPass.hpp"
#include"DE_Device.hpp"
#include"DE_Descriptors.hpp"
#include"DE_ControledCamera.hpp"
#include"DE_Renderer.hpp"
#include"DE_FrameData.hpp"
#include"Systems/PointLightSystem.hpp"
#include"Systems/DE_RenderSystem.hpp"
#include"DE_Swapchain.hpp"
#include<ranges>


namespace de{
	MainRenderPass::MainRenderPass(const Device& device,
		Renderer& renderer,
		RenderSystem& renderSystem,
		PointLightSystem& pointLightSys,
		ControlledCamera& camera,
		GameObject::Map& gameObjects,
		const DescriptorSetLayout& uboLayout)
		:renderer_{renderer},
		renderSystem_{renderSystem},
		pointLightSystem_{pointLightSys},
		camera_{camera},
		gameObjects_{gameObjects}{
	
		const auto framesInFlight = renderer_.getFramesInFlight();
		UBO.assign(framesInFlight, GlobalUBO{});

		pool_ = std::make_unique<de::DescriptorPool>(
			de::DescriptorPool::Builder(device)
			.addPoolSize(vk::DescriptorType::eUniformBuffer, framesInFlight)
			.build()
		);

		de::MemoryUsageInfo uboBufferInfo{
			sizeof(de::GlobalUBO),
			1,
			0,
			vk::BufferUsageFlagBits::eUniformBuffer,
			vk::MemoryPropertyFlagBits::eHostVisible
		};

		for (size_t i = 0; i < framesInFlight; ++i) {
			uboBuffers_.push_back(std::make_unique<Buffer>(device, uboBufferInfo));
		}

		descriptorSets_ = uboBuffers_
			| std::views::transform([&](const auto& uboBuffer) {

				uboBuffer->map();
				auto buffInfo = uboBuffer->getDescriptorInfo();
				return de::DescriptorWriter(device, *pool_, uboLayout)
					.writeBuffer(0, &buffInfo)
					.build();
			})
			| std::ranges::to<std::vector>();

	}

	MainRenderPass::~MainRenderPass() = default;
	
	std::vector<vk::ClearValue> MainRenderPass::getClearValues() const {
		return std::vector<vk::ClearValue>{
			vk::ClearValue{}.setColor(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}}),
			vk::ClearValue{}.setDepthStencil(vk::ClearDepthStencilValue{ 1.0f, 0 }) };
	}
	const RenderTarget& MainRenderPass::getRenderTarget() const{
		return renderer_.getSwapchainRenderTarget();
	}

	void MainRenderPass::updateTime(float deltaTime, float totalTime){
		deltaTime_ = deltaTime;
		totalTime = totalTime;
	}

	void MainRenderPass::record(const FrameData& frameData) {

		auto& currUBO = UBO[frameData.frameIndex];
		auto& rawCamera = camera_.getCamera();

		pointLightSystem_.update(gameObjects_, currUBO, deltaTime_, totalTime_);

		const auto extent = renderer_.getSwapchainExtent();
		const float aspectRatio = static_cast<float>(extent.width) / static_cast<float>(extent.height);
		currUBO.projectionMatrix = rawCamera.getProjectionMatrix(aspectRatio);
		currUBO.viewMatrix = rawCamera.getViewMatrix();
		currUBO.inverceViewMatrix = glm::inverse(currUBO.viewMatrix);

		uboBuffers_[frameData.frameIndex]->copyToBuffer(&currUBO, sizeof(GlobalUBO), 0);
		uboBuffers_[frameData.frameIndex]->flush();

		auto frameInfo = FrameInfo{
			.frameIndex = frameData.frameIndex,
			.commandBuffer = frameData.commandBuffer,
			.descriptorSet = descriptorSets_.at(frameData.frameIndex),
			.camera = camera_.getCamera(),
			.gameObjects = gameObjects_
		};

		renderSystem_.renderGameObjects(frameInfo);
		pointLightSystem_.render(frameInfo);
	}
	void MainRenderPass::onSwapchainRecreation(const Swapchain& sc){
		//skip, in this pass i use swapchainPass
		return;
	}
}