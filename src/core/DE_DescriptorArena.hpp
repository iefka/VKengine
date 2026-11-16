#pragma once

#include<cstdint>
#include<span>
#include<vector>
#include<memory>

#include<vulkan/vulkan.hpp>

namespace de{

    class Device;

    class DescriptorArena{
    public:
        DescriptorArena(const Device* device, uint32_t framesInFlight,
            std::span<const vk::DescriptorPoolSize> sizes, uint32_t maxSets);
        vk::DescriptorSet allocate(uint32_t frameIndex, vk::DescriptorSetLayout layout);

        //reset(frameIndex) is valid only while frameIndex`s fence has been signaled
        void reset(uint32_t frameIndex);
    private:
        const Device* device_;
        std::vector<vk::UniqueDescriptorPool> pools_;
    };
}