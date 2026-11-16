#pragma once

#include"DE_Device.hpp"
#include"DE_DescriptorSetLayoutCache.hpp"
#include"DE_DescriptorArena.hpp"
#include"DE_PipelineCache.hpp"

#include<vulkan/vulkan.hpp>

namespace de{
    struct RenderContext{
        Device* device;
        vk::Extent2D extent;
        vk::SampleCountFlagBits samples;
        const DescriptorSetLayoutCache* descriptorLayouts;
        DescriptorArena* descriptors;
        const PipelineCache* pipelinesCache;
    };
}
