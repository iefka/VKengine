#pragma once

#include<cstdint>

#include<vulkan/vulkan.hpp>

#include<utility>
#include<span>

namespace de{
    enum class SetSlot : uint32_t {eGlobal = 0, eTextures = 1,
         eMaterialData = 2, eTransforms = 3};

    consteval bool areConsecutiveFromZero(std::span<const SetSlot> slots) {
        for (uint32_t i = 0; i < slots.size(); ++i)
            if (std::to_underlying(slots[i]) != i) return false;
        return true;
    }
    // Binds a single descriptor set at its named slot.
    inline void bindSet(vk::CommandBuffer cmd,
             vk::PipelineLayout pipelineLayout,
             SetSlot slot,
             vk::DescriptorSet set,
             vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) {
        cmd.bindDescriptorSets(bindPoint, pipelineLayout, std::to_underlying(slot), { set }, {});
    }

    // Binds consecutive slots in one call (they must match the pipeline layout order).
    inline void bindSets(vk::CommandBuffer cmd,
                vk::PipelineLayout pipelineLayout,
                SetSlot first,
                std::span<const vk::DescriptorSet> sets,
                vk::PipelineBindPoint bindPoint = vk::PipelineBindPoint::eGraphics) {
        cmd.bindDescriptorSets(bindPoint, pipelineLayout, std::to_underlying(first), sets, {});
    }
}