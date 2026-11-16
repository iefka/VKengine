#include"DE_DescriptorArena.hpp"
#include"DE_Device.hpp"
#include"core/DE_Assert.hpp"

#include<cstdint>


namespace de{

    DescriptorArena::DescriptorArena(const Device* device, uint32_t framesInFlight,
        std::span<const vk::DescriptorPoolSize> sizes, uint32_t maxSets)
        :device_{device}{
        const auto& logicalDevice = device_->getLogicalDevice();

        const auto poolInfo = vk::DescriptorPoolCreateInfo{}
            .setMaxSets(maxSets)
            .setPoolSizes(sizes);

        pools_.reserve(framesInFlight);
        for (uint32_t i = 0; i < framesInFlight; ++i){
            pools_.push_back(
                logicalDevice.createDescriptorPoolUnique(poolInfo)
            );
        }
            
    }
    
    vk::DescriptorSet DescriptorArena::allocate(uint32_t frameIndex, 
        vk::DescriptorSetLayout layout){
        DE_ASSERT(frameIndex < pools_.size() && "frame index out of range");

        const auto& logicalDevice = device_->getLogicalDevice();

        const auto DSAllocInfo = vk::DescriptorSetAllocateInfo{}
            .setDescriptorPool(*pools_.at(frameIndex))
            .setSetLayouts(layout);

        return logicalDevice.allocateDescriptorSets(DSAllocInfo).front(); 
    }

    void DescriptorArena::reset(uint32_t frameIndex){
        DE_ASSERT(frameIndex < pools_.size() && "frame index out of range");

        const auto& logicalDevice = device_->getLogicalDevice();
        logicalDevice.resetDescriptorPool(*pools_.at(frameIndex));
    }
}