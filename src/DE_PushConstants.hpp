#pragma once
#include <vulkan/vulkan.hpp>
#include <vector>
#include <cstring>

namespace de {

    template<typename T>
    class PushConstant {
    public:
        PushConstant(vk::ShaderStageFlags stages, uint32_t offset = 0)
            : range_(vk::PushConstantRange{}
                .setStageFlags(stages)
                .setOffset(offset)
                .setSize(sizeof(T)))
        {}

        void setValue(const T& value) {
            data_ = value;
        }

        const T& getValue() const {
            return data_;
        }

        const vk::PushConstantRange& getRange() const {
            return range_;
        }

        void push(vk::CommandBuffer commandBuffer, vk::PipelineLayout layout) const {
            commandBuffer.pushConstants(
                layout,
                range_.stageFlags,
                range_.offset,
                sizeof(T),
                &data_
            );
        }

    private:
        T data_{};
        vk::PushConstantRange range_;
    };

} // namespace de