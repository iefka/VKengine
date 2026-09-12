#pragma once

#include"DE_RenderTarget.hpp"
#include<vulkan/vulkan.hpp>
#include<vector>

namespace de {
	struct FrameData;
	class Swapchain;

	class IRenderPass {
	public:
		virtual ~IRenderPass() = default;
		virtual std::vector<vk::ClearValue> getClearValues() const = 0;
		virtual const RenderTarget& getRenderTarget() const = 0;
		virtual void record(const FrameData& frameData) = 0;
		virtual void onSwapchainRecreation(const Swapchain& sc) {};

	};
}