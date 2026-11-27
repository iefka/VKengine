#pragma once

#include<vulkan/vulkan.hpp>
#include<optional>

namespace de {
	struct FrameData;
	class Swapchain;

	struct DepthAttachment{
		vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eLoad;
		vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eDontCare;
		vk::ClearValue clear{};
	};
	struct ColorAttachment{
		vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eLoad;
		vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eDontCare;
		vk::ClearValue clear{};
	};

	class IRenderPass {
	public:
		virtual ~IRenderPass() = default;
		virtual ColorAttachment colorClearValue() const {return {};}
		virtual std::optional<DepthAttachment> depthClearValue() const {return std::nullopt;}
		virtual void record(const FrameData& frameData) = 0;
		virtual bool useDepth() const {return true;}
	};
}