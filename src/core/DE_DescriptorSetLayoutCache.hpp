#pragma once

#include"core/DE_Assert.hpp"
#include"DE_SlotUtils.hpp"

#include<unordered_map>
#include<type_traits>
#include<utility>
#include<array>



namespace de{

     

    class DescriptorSetLayoutCache{
    public:
        
        void ensure(details::Uint32EnumClass auto slot, vk::DescriptorSetLayout layout){
            const auto [it, inserted] = layouts_.try_emplace(std::to_underlying(slot), layout);
            DE_ASSERT(inserted || it->second == layout);   // same slot => same layout, forever
        }

         vk::DescriptorSetLayout get(details::Uint32EnumClass auto slot) const {
            const auto it = layouts_.find(std::to_underlying(slot));
            DE_ASSERT(it != layouts_.end() && "DescriptorSetLayoutCache: slot not filled");
            
            return it->second;
        }
    private:
        std::unordered_map<uint32_t, vk::DescriptorSetLayout> layouts_;
    };

}