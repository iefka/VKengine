#ifndef _DE_COMPUTE_TEXTURE_TARGET_
#define _DE_COMPUTE_TEXTURE_TARGET_

#include <vulkan/vulkan.hpp>
#include <vector>
#include <memory>
#include <filesystem>

#include "DE_IComputeStep.hpp"
#include "DE_PushConstants.hpp"
#include "DE_ITextureSource.hpp"

namespace de {

    class Device;
    class Image;
    class Shader;
    class DescriptorSetLayout;
    class DescriptorPool;
    struct ComputeTag;
    template <typename Tag> class Pipeline;
    using ComputePipeline = Pipeline<ComputeTag>;

    struct ComputeTextureTargetPush {
        float pcTime;
    };


    class ComputeTextureTarget final : public IComputeStep {
    public:
        ComputeTextureTarget(const Device& device,
            vk::Extent2D extent,
            uint32_t framesInFlight,
            const std::filesystem::path& computeShaderPath);
        ~ComputeTextureTarget();

        ComputeTextureTarget(const ComputeTextureTarget&) = delete;
        ComputeTextureTarget& operator=(const ComputeTextureTarget&) = delete;

        void updateTime(float totalTime);

        void dispatch(vk::CommandBuffer commandBuffer, uint32_t frameIndex) override;

        const ITextureSource& getTextureSource(uint32_t frameIndex) const;

        uint32_t getFramesInFlight() const noexcept { return framesInFlight_; }

    private:
        void createPipeline();

        const Device& device_;
        vk::Extent2D extent_;
        uint32_t framesInFlight_;

        std::unique_ptr<Shader> computeShader_;
        std::unique_ptr<ComputePipeline> computePipeline_;
        std::unique_ptr<DescriptorSetLayout> descriptorLayout_;
        std::unique_ptr<DescriptorPool> descriptorPool_;

        vk::UniqueSampler sampler_;

        struct FrameSlot {
            std::unique_ptr<Image> image;
            vk::UniqueImageView view;
            vk::DescriptorSet descriptorSet;
            std::unique_ptr<ComputeTextureView> textureSource;
        };
        std::vector<FrameSlot> slots_;

        ComputeTextureTargetPush pushConstantData_{};
        PushConstant<ComputeTextureTargetPush> pushConstant_{ vk::ShaderStageFlagBits::eCompute };
    };

}

#endif
