#include "DE_DescriptorSetLayoutCache.hpp"
#include"core/DE_Assert.hpp"
namespace de{

    void DescriptorSetLayoutCache::fillSlot(SetSlot slot, vk::DescriptorSetLayout layout){
        [[maybe_unused]] auto [it, inserted] = 
            layouts_.try_emplace(slot, std::move(layout));

        DE_ASSERT(inserted && "slot already filled");
    }

    
    vk::DescriptorSetLayout DescriptorSetLayoutCache::get(SetSlot slot) const {
        const auto it = layouts_.find(slot);
        if (it == layouts_.end())
            throw std::logic_error("DescriptorSetLayoutCache: slot not filled");
        return it->second;
    }

}