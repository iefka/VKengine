#ifndef _DE_RENDER_PASS_
#define _DE_RENDER_PASS_

#include <vulkan/vulkan.hpp>
#include <vector>

namespace de {
	class Device;

	class RenderPass {
	public:
        friend class Builder;
		class Builder {
		public:
			explicit Builder(const Device& device) : device_(device) {}

			// Set a single color attachment for the render pass (simple common case)
			Builder& setColorAttachment(vk::Format format,
										vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1,
										vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear,
										vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eStore,
										vk::ImageLayout initial = vk::ImageLayout::eUndefined,
										vk::ImageLayout finalLayout = vk::ImageLayout::ePresentSrcKHR);

			// Optionally set a depth attachment
			Builder& setDepthAttachment(vk::Format format,
										vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1,
										vk::AttachmentLoadOp loadOp = vk::AttachmentLoadOp::eClear,
										vk::AttachmentStoreOp storeOp = vk::AttachmentStoreOp::eDontCare,
										vk::ImageLayout initial = vk::ImageLayout::eUndefined,
										vk::ImageLayout finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal);

			RenderPass build() const;

		private:
			const Device& device_;

            // Store attachment descriptions directly
            std::vector<vk::AttachmentDescription> attachments_; 
			// Color attachment references (indices correspond to attachments_)
            std::vector<vk::AttachmentReference> colorAttachmentRefs_; 
			// Depth attachment reference if present
            bool hasDepth_ = false; 
			vk::AttachmentReference depthAttachmentRef_{};
            friend class RenderPass;
		};

        // Prevent direct construction; use Builder::build()
		RenderPass() = delete;

		// Allow move semantics
		RenderPass(RenderPass&&) = default;
		RenderPass& operator=(RenderPass&&) = default;

		// Access underlying Vulkan render pass
		const vk::UniqueRenderPass& getRenderPass() const noexcept { return renderPass_; }

	private:
        // Construct from Builder (used by Builder)
		explicit RenderPass(const Builder& builder);

		vk::UniqueRenderPass renderPass_;
		const Device& device_;
	};
}

#endif