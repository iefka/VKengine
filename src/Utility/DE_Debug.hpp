#ifndef _DE_DEBUG_
#define _DE_DEBUG_

//std
#include<iostream>
//vk
#include<vulkan/vulkan.hpp>

namespace de::debug{

	inline void printPhysicalDeviceProperties(vk::PhysicalDevice physicalDevice){
		
        const auto props = physicalDevice.getProperties();
        const auto features = physicalDevice.getFeatures();
        std::cout <<
            "    " << props.deviceName << ":" <<
            "\n      is discrete GPU: " << (props.deviceType == vk::PhysicalDeviceType::eDiscreteGpu ? "yes, " : "no, ") <<
            "\n      has geometry shader: " << (features.geometryShader ? "yes, " : "no, ") <<
            "\n      has tesselation shader: " << (features.tessellationShader ? "yes, " : "no, ") <<
            "\n      supports anisotropic filtering: " << (features.samplerAnisotropy ? "yes, " : "no, ") <<
            "\n";
	}

    inline void printQueueFamilyProperties(const std::vector<vk::QueueFamilyProperties> props){
        for (auto&& family : props)
        {
            std::cout <<
                "\n    Queue Family " << "-----------------------------" << ":\n" <<
                "\n        queue count: " << family.queueCount <<
                "\n        supports graphics operations: " << (family.queueFlags & vk::QueueFlagBits::eGraphics ? "yes" : "no") <<
                "\n        supports compute operations: " << (family.queueFlags & vk::QueueFlagBits::eCompute ? "yes" : "no") <<
                "\n        supports transfer operations: " << (family.queueFlags & vk::QueueFlagBits::eTransfer ? "yes" : "no") <<
                "\n        supports sparse binding operations: " << (family.queueFlags & vk::QueueFlagBits::eSparseBinding ? "yes" : "no") <<
                "\n";
        }
    }
}
#endif