#ifndef _DE_DEBUG_
#define _DE_DEBUG_

//std
#include<iostream>
#include <print>   // Требуется C++23 (для std::print / std::println)
#include <string_view>
//vk
#include<vulkan/vulkan.hpp>

namespace de::debug{


    [[maybe_unused]] static VKAPI_ATTR vk::Bool32 VKAPI_CALL MyDebugCallback(
        vk::DebugUtilsMessageSeverityFlagBitsEXT           messageSeverity,
        vk::DebugUtilsMessageTypeFlagsEXT                  messageTypes,
        const vk::DebugUtilsMessengerCallbackDataEXT*      pCallbackData,
        void*                                            /*pUserData*/) //no data yet
    {
        // 1. Выбираем цвет и тег важности (используя std::string_view без лишних аллокаций)
        std::string_view severityStr = "[UNKNOWN]";
        if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eError) {
            severityStr = "\033[1;31m[ERROR]\033[0m"; // Красный
        } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
            severityStr = "\033[1;33m[WARNING]\033[0m"; // Желтый
        } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eInfo) {
            severityStr = "\033[1;36m[INFO]\033[0m"; // Бирюзовый
        } else if (messageSeverity & vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose) {
            severityStr = "[VERBOSE]";
        }

        // 2. Выбираем тип сообщения
        std::string_view typeStr = "";
        if (messageTypes &  vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation) {
            typeStr = "(Validation)";
        } else if (messageTypes & vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance) {
            typeStr = "(Performance)";
        } else if (messageTypes & vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral) {
            typeStr = "(General)";
        }

        // 3. Выводим напрямую в stderr с помощью std::println (C++23)
        // {} автоматически подставит string_view и обычный char* из Vulkan
        std::println(stderr, "{} {} {}", severityStr, typeStr, pCallbackData->pMessage);

        return VK_FALSE;
    }


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