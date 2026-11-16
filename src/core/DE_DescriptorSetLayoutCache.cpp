#include "DE_DescriptorSetLayoutCache.hpp"
#include"core/DE_Assert.hpp"
namespace de{

    void DescriptorSetLayoutCache::ensure(SetSlot slot, vk::DescriptorSetLayout layout) {
        const auto [it, inserted] = layouts_.try_emplace(slot, layout);
        DE_ASSERT(inserted || it->second == layout);   // same slot => same layout, forever

    }

    
    vk::DescriptorSetLayout DescriptorSetLayoutCache::get(SetSlot slot) const {
        const auto it = layouts_.find(slot);
        DE_ASSERT(it == layouts_.end() && "DescriptorSetLayoutCache: slot not filled");
           
        return it->second;
    }

}