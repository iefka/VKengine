#pragma once

#include"DE_IRenderPass.hpp"
#include"DE_GameObject.hpp"
#include<memory>

namespace de {

	class RenderSystem;
	class PointLightSystem;
	class Renderer;
	class Buffer;
	class DescriptorPool;
	class Swapchain;
	class ControlledCamera;
	class Device;
	class DescriptorSetLayout;
	class GlobalUBO;

	class MainRenderPass : public IRenderPass{
	public:
		MainRenderPass(const Device& device,
			Renderer& renderer,
			RenderSystem& renderSystem,
			PointLightSystem& pointLightSys,
			ControlledCamera& camera,
			GameObject::Map& gameObjects,
			const DescriptorSetLayout& uboLayout);
		~MainRenderPass();



		 void updateTime(float deltaTime, float totalTime);
		 std::vector<vk::ClearValue> getClearValues()  const override;
		 const RenderTarget& getRenderTarget() const override;
		 void record(const FrameData& frameData) override;
		 void onSwapchainRecreation(const Swapchain& sc) override;

	private:

		RenderSystem& renderSystem_;
		PointLightSystem& pointLightSystem_;
		Renderer& renderer_;
		ControlledCamera& camera_;
		GameObject::Map& gameObjects_;

		std::vector<GlobalUBO> UBO{};
		std::unique_ptr<DescriptorPool> pool_{};
		std::vector<std::unique_ptr<Buffer>> uboBuffers_{};
		std::vector<vk::DescriptorSet> descriptorSets_{};

		float deltaTime_;
		float totalTime_;
	};
}