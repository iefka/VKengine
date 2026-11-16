#pragma once

#include<cstdint>

#include<vulkan/vulkan.hpp>

namespace de{
    enum class SetSlot : uint32_t {eGlobal = 0, eTextures = 1,
         eMaterialData = 2, eTransform = 3};
    inline constexpr uint32_t kSetSlotCount = 4;

// Binds a single descriptor set at its named slot.
void bindSet(vk::CommandBuffer cmd,
             vk::PipelineLayout pipelineLayout,
             SetSlot slot,
             vk::DescriptorSet set,
             vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) {
    cmd.bindDescriptorSets(bindPoint, pipelineLayout, std::to_underlying(slot), { set }, {});
}

// Binds consecutive slots in one call (they must match the pipeline layout order).
void bindSets(vk::CommandBuffer cmd,
              vk::PipelineLayout pipelineLayout,
              SetSlot first,
              std::span<const vk::DescriptorSet> sets,
              vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) {
    cmd.bindDescriptorSets(bindPoint, pipelineLayout, std::to_underlying(first), sets, {});
}
}