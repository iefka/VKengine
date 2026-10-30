#include "DE_Device.hpp"
#include "DE_Descriptors.hpp"
#include "Utility/DE_Utility.hpp"
#include "DE_Memory.hpp"
#include "DE_Shaders.hpp"
#include "DE_Pipeline.hpp"
#include"DE_ShaderTexturingPass.hpp"

namespace de {

    namespace {

        constexpr vk::Format kImageFormat = vk::Format::eR8G8B8A8Unorm;
    }

    ComputeTextureTarget::ComputeTextureTarget(const Device& device,
        vk::Extent2D extent,
        uint32_t framesInFlight,
        const std::filesystem::path& computeShaderPath)
        : device_{ device }
        , extent_{ extent }
        , framesInFlight_{ framesInFlight }
    {
        computeShader_ = std::make_unique<Shader>(device_, computeShaderPath);

        descriptorLayout_ = std::make_unique<DescriptorSetLayout>(
            DescriptorSetLayout::Builder{ device_ }
            .addBinding(0, vk::DescriptorType::eStorageImage, vk::ShaderStageFlagBits::eCompute)
            .build()
        );

        descriptorPool_ = std::make_unique<DescriptorPool>(
            DescriptorPool::Builder{ device_ }
            .setMaxSets(framesInFlight_)
            .addPoolSize(vk::DescriptorType::eStorageImage, framesInFlight_)
            .build()
        );

       
        sampler_ = utl::createTextureSampler(device_.getLogicalDevice(),
            vk::SamplerAddressMode::eClampToEdge, false);

        slots_.resize(framesInFlight_);
        for (uint32_t i = 0; i < framesInFlight_; ++i) {

            auto imageInfo = ImageUsageInfo{
                0, 0,
                vk::ImageCreateInfo{}
                    .setImageType(vk::ImageType::e2D)
                    .setExtent(vk::Extent3D{ extent_, 1 })
                    .setMipLevels(1)
                    .setArrayLayers(1)
                    .setTiling(vk::ImageTiling::eOptimal)
                    .setUsage(vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled)
                    .setFormat(kImageFormat)
                    .setInitialLayout(vk::ImageLayout::eUndefined)
                    .setSamples(vk::SampleCountFlagBits::e1)
                    .setSharingMode(vk::SharingMode::eExclusive),
                vk::MemoryPropertyFlagBits::eDeviceLocal
            };

            slots_[i].image = std::make_unique<Image>(device_, imageInfo);

            device_.transitionImageLayout(slots_[i].image->getImage(),
                kImageFormat, vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

            slots_[i].view = utl::createImageView(device_.getLogicalDevice(), slots_[i].image->getImage(),
                kImageFormat, vk::ImageAspectFlagBits::eColor);

            auto descriptorImageInfo = vk::DescriptorImageInfo{}
                .setImageLayout(vk::ImageLayout::eGeneral)
                .setImageView(*slots_[i].view)
                .setSampler(*sampler_);

            slots_[i].descriptorSet = DescriptorWriter{ device_, *descriptorPool_, *descriptorLayout_ }
                .writeImage(0, &descriptorImageInfo)
                .build();

            slots_[i].textureSource = std::make_unique<ComputeTextureView>(
                *sampler_, *slots_[i].view, vk::ImageLayout::eGeneral);
        }

        createPipeline();
    }

    ComputeTextureTarget::~ComputeTextureTarget() = default;

    void ComputeTextureTarget::updateTime(float totalTime) {
        pushConstantData_.pcTime = totalTime;
    }

    void ComputeTextureTarget::dispatch(vk::CommandBuffer commandBuffer, uint32_t frameIndex) {

        pushConstant_.setValue(pushConstantData_);

        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, *computePipeline_->getPipeline());
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
            *computePipeline_->getLayout(), 0, slots_[frameIndex].descriptorSet, {});
        pushConstant_.push(commandBuffer, *computePipeline_->getLayout());

        const uint32_t groupX = (extent_.width + 15) / 16;
        const uint32_t groupY = (extent_.height + 15) / 16;
        commandBuffer.dispatch(groupX, groupY, 1);

        
        auto barrier = vk::ImageMemoryBarrier{}
            .setOldLayout(vk::ImageLayout::eGeneral)
            .setNewLayout(vk::ImageLayout::eGeneral)
            .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setDstAccessMask(vk::AccessFlagBits::eShaderRead)
            .setImage(slots_[frameIndex].image->getImage())
            .setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eFragmentShader,
            {}, {}, {}, barrier);
    }

    const ITextureSource& ComputeTextureTarget::getTextureSource(uint32_t frameIndex) const {
        return *slots_[frameIndex].textureSource;
    }

    void ComputeTextureTarget::createPipeline() {

        auto builder = ComputePipeline::Builer{}
            .setShaderStage(
                vk::PipelineShaderStageCreateInfo{}
                .setModule(computeShader_->getShaderModule())
                .setPName("main")
                .setStage(vk::ShaderStageFlagBits::eCompute)
            )
            .addDsLayout(descriptorLayout_->getLayout())
            .setPushConstantRanges({ pushConstant_.getRange() });

        computePipeline_ = std::make_unique<ComputePipeline>(device_, builder);
    }

} // namespace de
