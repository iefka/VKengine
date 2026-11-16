#include"DE_PipelineCache.hpp"
#include"DE_Device.hpp"

#include<span>
#include<fstream>
#include<cstdint>
#include<exception>
#include<print>

namespace de{

    PipelineCache::PipelineCache(const Device* device)
    :device_{device}{
        const auto& logicalDevice = device_->getLogicalDevice();
       
        std::vector<std::byte> initialData;
        std::error_code ec;
        const auto size = std::filesystem::file_size(filePath_, ec);
        
        if(!ec && size > 0){
            std::ifstream file(filePath_,std::ios::binary);
            initialData.resize(size);
            file.read(reinterpret_cast<char*>(
                initialData.data()),
                static_cast<std::streamsize>(size)
            );
        }else{
            std::println("PipelineCache: no usable cache at {}, starting cold", filePath_.string());
        }

        vk::PipelineCacheCreateInfo cacheCreateInfo{};
        if (!initialData.empty()) {
            cacheCreateInfo.setInitialData(vk::ArrayProxyNoTemporaries<const std::byte>(initialData));
            std::println("PipelineCache: loaded {} bytes from {}", initialData.size(), filePath_.string());
        }

        cache_ = logicalDevice.createPipelineCacheUnique(cacheCreateInfo);

    }

    PipelineCache::~PipelineCache() {
        if (!cache_) return;
        try {
            const auto data = device_->getLogicalDevice().getPipelineCacheData(*cache_);
            if (data.empty()) return;

            std::error_code ec;
            std::filesystem::create_directories(filePath_.parent_path(), ec);

            std::ofstream file(filePath_, std::ios::binary | std::ios::trunc);
            file.write(reinterpret_cast<const char*>(data.data()),
                    static_cast<std::streamsize>(data.size()));

            if (!file)
                std::println("PipelineCache: write failed: {}", filePath_.string());
            else
                std::println("PipelineCache: saved {} bytes", data.size());
        } catch (const std::exception& e) {
            std::println("PipelineCache: save failed: {}", e.what());
        }
    }
}