#pragma once

#include "DE_IRenderPass.hpp"
#include<memory>
#include"DE_PushConstants.hpp"
#include"DE_RenderTarget.hpp"
#include<filesystem>

namespace de {

	class Device;
	class Renderer;
	class Image;
	class Shader;
	class DescriptorSetLayout;
	class DescriptorPool;
	struct ComputeTag;
	struct GraphicsTag;
	template <typename Tag> class Pipeline;
	using ComputePipeline = Pipeline<ComputeTag>;
	using GraphicsPipeline = Pipeline<GraphicsTag>;

	struct ShaderTexturingPush{
		float pcTime;
	};

	class ShaderTexturingPass final : public IRenderPass{
	public:
		ShaderTexturingPass(const Device& device,
			vk::Extent2D extent,
			const RenderPass& renderPass,
			const Renderer& renderer,
			const std::filesystem::path& computeShaderPath);
		~ShaderTexturingPass();


		void updateTime(float deltaTime);
		std::vector<vk::ClearValue> getClearValues()  const override;
		const RenderTarget& getRenderTarget() const override;
		void preRecord(const FrameData& frameData) override;
		void record(const FrameData& frameData) override;
		void onSwapchainRecreation(const Swapchain& sc) override;

	private:

		void createPipelines(vk::Extent2D extent);

		const Device& device_;
		const Renderer& renderer_;
		const RenderPass& renderPass_;
		vk::Extent2D extent_;

		std::unique_ptr<ComputePipeline> computePipeline_;
		std::unique_ptr<GraphicsPipeline> graphicsPipeline_;

		std::unique_ptr<Image> computeImage_;
		vk::UniqueImageView computeImageView_;
		vk::UniqueSampler computeImageSampler_;

		std::unique_ptr<DescriptorSetLayout> computeDescriptorLayout_;
		std::unique_ptr<DescriptorSetLayout> graphicsDescriptorLayout_;
		std::unique_ptr<DescriptorPool> descriptorPool_;
		vk::DescriptorSet computeDescriptor_;
		vk::DescriptorSet graphicsDescriptor_;

		std::unique_ptr<Shader> vertexShader_;
		std::unique_ptr<Shader> fragmentShader_;
		std::unique_ptr<Shader> computeShader_;

		ShaderTexturingPush pushConstantData_;
		PushConstant<ShaderTexturingPush> pushConstant_{vk::ShaderStageFlagBits::eCompute};

	};
}