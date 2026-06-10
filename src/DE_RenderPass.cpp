#include "DE_Device.hpp"
#include "DE_RenderPass.hpp"


namespace de {

    // Builder implementation
   

    RenderPass::Builder& RenderPass::Builder::setColorAttachment(vk::Format format,
        vk::SampleCountFlagBits samples, vk::AttachmentLoadOp loadOp,
        vk::AttachmentStoreOp storeOp, vk::ImageLayout initial,
        vk::ImageLayout finalLayout) {

        const auto colorAttach = vk::AttachmentDescription{}
            .setFormat(format)
            .setSamples(samples)
            .setLoadOp(loadOp)
            .setStoreOp(storeOp)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(initial)
            .setFinalLayout(finalLayout);
        attachments_.push_back(colorAttach);

        const auto attachmentRef = vk::AttachmentReference{}
            .setAttachment(static_cast<uint32_t>(attachments_.size() - 1))
            .setLayout(vk::ImageLayout::eColorAttachmentOptimal);
        colorAttachmentRefs_.push_back(attachmentRef);
        return *this;
    }

    RenderPass::Builder& RenderPass::Builder::setDepthAttachment(vk::Format format,
        vk::SampleCountFlagBits samples,
        vk::AttachmentLoadOp loadOp,
        vk::AttachmentStoreOp storeOp,
        vk::ImageLayout initial,
        vk::ImageLayout finalLayout) {

        const auto depthAttach = vk::AttachmentDescription{}
            .setFormat(format)
            .setSamples(samples)
            .setLoadOp(loadOp)
            .setStoreOp(storeOp)
            .setStencilLoadOp(vk::AttachmentLoadOp::eDontCare)
            .setStencilStoreOp(vk::AttachmentStoreOp::eDontCare)
            .setInitialLayout(initial)
            .setFinalLayout(finalLayout);
        attachments_.push_back(depthAttach);
        hasDepth_ = true;
        depthAttachmentRef_.attachment = static_cast<uint32_t>(attachments_.size() - 1);
        depthAttachmentRef_.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        return *this;
    }

    RenderPass RenderPass::Builder::build() const {
        return RenderPass(*this);
    }

    // RenderPass implementation
    RenderPass::RenderPass(const Builder& builder) : device_(builder.device_) {
        const auto& device = device_.getLogicalDevice();

        // Use attachments and references collected in builder
        const auto& attachments = builder.attachments_;
        const auto& colorRefs = builder.colorAttachmentRefs_;
        vk::AttachmentReference depthRef = builder.depthAttachmentRef_;

        auto subpass = vk::SubpassDescription{}
            .setPipelineBindPoint(vk::PipelineBindPoint::eGraphics)
            .setColorAttachments(colorRefs);
        if (builder.hasDepth_) subpass.setPDepthStencilAttachment(&depthRef);

        // Subpass dependency to wait for the swapchain to finish reading
        const auto dependency = vk::SubpassDependency{}
            .setSrcSubpass(VK_SUBPASS_EXTERNAL)
            .setDstSubpass(0)
            .setSrcStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setSrcAccessMask({})
            .setDstStageMask(vk::PipelineStageFlagBits::eColorAttachmentOutput)
            .setDstAccessMask(vk::AccessFlagBits::eColorAttachmentWrite);

        const auto renderPassInfo = vk::RenderPassCreateInfo{}
            .setAttachments(attachments)
            .setSubpassCount(1)
            .setPSubpasses(&subpass)
            .setDependencyCount(1)
            .setPDependencies(&dependency);

        renderPass_ = device.createRenderPassUnique(renderPassInfo);
    }

}
