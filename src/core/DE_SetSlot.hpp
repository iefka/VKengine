#pragma once

#include<cstdint>

#include<vulkan/vulkan.hpp>

#include<span>

namespace de{
    enum class SetSlot : uint32_t {eGlobal = 0, eTextures = 1,
         eMaterialData = 2, eTransforms = 3};

    // Binds consecutive slots in one call (they must match the pipeline layout order).
    inline void bindSets(vk::CommandBuffer cmd,
                vk::PipelineLayout pipelineLayout,
                std::span<const vk::DescriptorSet> sets,
                vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) {
        cmd.bindDescriptorSets(bindPoint, pipelineLayout, 0, sets, {});
    }
}