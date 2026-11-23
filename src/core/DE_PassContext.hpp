#pragma once

#include"DE_RenderPass.hpp"
#include<vulkan/vulkan.hpp>


namespace de{
  
    struct PassContext{
        vk::Format colorFormat;
        vk::Format depthFormat;
        vk::Extent2D extent;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    };
}