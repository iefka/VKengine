#pragma once
#include<vulkan/vulkan.hpp>

#include<filesystem>

namespace de{
    class Device;

    class PipelineCache{
    public:
        PipelineCache(const Device* device);
        ~PipelineCache();
        vk::PipelineCache getCache() const noexcept {return *cache_;}
    private:
        const Device* device_;
        const std::filesystem::path filePath_ = std::filesystem::current_path() / "Cache/pipeline_cache.bin";
        vk::UniquePipelineCache cache_;
    };
}