#pragma once

#include"DE_RenderPass.hpp"
#include<vulkan/vulkan.hpp>


namespace de{
  
    struct PassContext{
        RenderPass* renderPass;
        vk::Framebuffer frameBuffer;
        vk::Extent2D extent;
        vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1;
    };
}