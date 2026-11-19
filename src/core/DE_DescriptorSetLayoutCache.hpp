#pragma once

#include"DE_Descriptors.hpp"
#include"core/DE_Assert.hpp"

#include<unordered_map>
#include<type_traits>
#include<utility>



namespace de{

    namespace details{
        template<typename T>
        concept Uint32EnumClass = std::is_scoped_enum_v<T> 
        && std::is_same_v<std::underlying_type_t<T>, std::uint32_t>;
    }

    template<details::Uint32EnumClass Slot, std::size_t N>
        consteval bool areConsecutiveFromZero(const std::array<Slot, N>& slots) {
            for (uint32_t i = 0; i < N; ++i)
                if (std::to_underlying(slots[i]) != i) return false;
        return true;
        }   

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