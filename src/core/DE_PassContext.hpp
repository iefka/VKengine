#pragma once

#include<vulkan/vulkan.hpp>

namespace de{
  
    constexpr vk::Format kDepthFormat = vk::Format::eD32Sfloat;

    struct PassContext{
        vk::Format colorFormat;
        vk::Format depthFormat = kDepthFormat;
        vk::Extent2D extent;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    };
}