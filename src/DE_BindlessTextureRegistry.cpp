#include"DE_Device.hpp"
#include "DE_BindlessTextureRegistry.hpp"

namespace de {
	BindlessTextureRegistry::BindlessTextureRegistry(const Device& device, uint32_t maxTextures)
		:device_{device}
        ,maxTextures_{ maxTextures } {
	
		const auto& logicalDevice = device.getLogicalDevice();

        const auto binding = vk::DescriptorSetLayoutBinding{}
            .setBinding(0)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(maxTextures_)
            .setStageFlags(vk::ShaderStageFlagBits::eFragment);

        const vk::DescriptorBindingFlags bindingFlags =
            vk::DescriptorBindingFlagBits::ePartiallyBound
            | vk::DescriptorBindingFlagBits::eUpdateAfterBind
            | vk::DescriptorBindingFlagBits::eVariableDescriptorCount;

        auto bindingFlagsInfo = vk::DescriptorSetLayoutBindingFlagsCreateInfo{}
            .setBindingFlags(bindingFlags);

        const auto layoutInfo = vk::DescriptorSetLayoutCreateInfo{}
            .setBindings(binding)
            .setFlags(vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool)
            .setPNext(&bindingFlagsInfo);

        layout_ = logicalDevice.createDescriptorSetLayoutUnique(layoutInfo);

        const auto poolSize = vk::DescriptorPoolSize{}
            .setType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(maxTextures_);

        const auto poolInfo = vk::DescriptorPoolCreateInfo{}
            .setPoolSizes(poolSize)
            .setMaxSets(1)
            .setFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind);

        pool_ = logicalDevice.createDescriptorPoolUnique(poolInfo);

    
        const uint32_t variableCount = maxTextures_;
        auto variableCountInfo = vk::DescriptorSetVariableDescriptorCountAllocateInfo{}
            .setDescriptorCounts(variableCount);

        const auto allocInfo = vk::DescriptorSetAllocateInfo{}
            .setDescriptorPool(*pool_)
            .setSetLayouts(*layout_)
            .setPNext(&variableCountInfo);

        auto sets = logicalDevice.allocateDescriptorSets(allocInfo);
        descriptorSet_ = sets[0];

	}

    uint32_t BindlessTextureRegistry::registerTexture(const ITextureSource& texture) {
        if (nextFreeIndex_ >= maxTextures_) {
            throw std::runtime_error("BindlessTextureRegistry: out of texture slots");
        }

        const uint32_t index = nextFreeIndex_++;

        const auto imageInfo = texture.getDescriptorInfo();
        const auto write = vk::WriteDescriptorSet{}
            .setDstSet(descriptorSet_)
            .setDstBinding(0)
            .setDstArrayElement(index)
            .setDescriptorType(vk::DescriptorType::eCombinedImageSampler)
            .setDescriptorCount(1)
            .setImageInfo(imageInfo);
        device_.getLogicalDevice().updateDescriptorSets(write,{});
        return index;
    }
}
