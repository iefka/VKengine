#pragma once

#include"DE_SetSlot.hpp"
#include"DE_Descriptors.hpp"

#include<unordered_map>

namespace de{
    class DescriptorSetLayoutCache{
    public:
        void ensure(SetSlot slot, vk::DescriptorSetLayout layout);
         vk::DescriptorSetLayout get(SetSlot slot) const ;
    private:
        std::unordered_map<SetSlot, vk::DescriptorSetLayout> layouts_;
    };

}