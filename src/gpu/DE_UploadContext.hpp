#pragma once

#include<cstddef>
#include<span>
#include<memory>
#include<vector>

#include<vulkan/vulkan.hpp>

#include"core/DE_Assert.hpp"
#include"DE_Memory.hpp"

namespace de{
    class Device;

    class UploadContext{
    public:
        explicit UploadContext(const Device& device,
            vk::DeviceSize stagingCapacity = 16ull * 1024 * 1024);
        ~UploadContext();        

        UploadContext(const UploadContext&) = delete;
        UploadContext& operator=(const UploadContext&) = delete;

        void begin();
        void flush();

        void copyBuffer(vk::Buffer dst, std::span<const std::byte> src,
            vk::DeviceSize dstOffset = 0);

        
        void copyBufferToImage(vk::Image dst, std::span<const std::byte> src,
            vk::Extent3D extent, uint32_t mipLevel = 0, uint32_t layer = 0);

        void transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout, uint32_t layerCount = 1, uint32_t levelCount = 1);

                // Escape hatch
        vk::CommandBuffer cmd() const;

        // delete resource after butch end (grow: old buffer still need gpu).
        void retire(std::unique_ptr<Buffer> buffer);

        bool isRecording() const noexcept{ return recording_; }

    private:
        vk::DeviceSize allocate(vk::DeviceSize size);
        void growStaging(vk::DeviceSize minSize);

        const Device& device_;
        vk::DeviceSize stagingCapacity_;
        std::unique_ptr<Buffer> staging_;
        vk::DeviceSize stagingUsed_ = 0;

        vk::UniqueCommandPool pool_;
        vk::CommandBuffer cmd_;
        vk::UniqueFence fence_;
        bool recording_ = false;

        std::vector<vk::Buffer> writtenBuffers_;
        std::vector<std::unique_ptr<Buffer>> retired_;
    };
}