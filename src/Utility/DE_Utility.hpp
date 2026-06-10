#pragma once
#ifndef _DE_UTILITY_
#define _DE_UTILITY_


#include<vulkan/vulkan.hpp>
namespace de {
	namespace utl {

		inline uint32_t getVertexFormatSize(vk::Format format) {
			switch (format)
			{
			case vk::Format::eR32G32B32A32Sfloat:
				return static_cast<uint32_t> (4 * sizeof(float));

			default:
				throw std::invalid_argument("unsuported vertex format");
			}
		}

		inline uint32_t getSuitableMemoryIndex(const vk::PhysicalDeviceMemoryProperties& memoryProperties,
			std::uint32_t allowedTypesMask, vk::MemoryPropertyFlags requiredMemoryFlags) {

			for (std::uint32_t i = 0; i < memoryProperties.memoryTypeCount; ++i) {
				if ((allowedTypesMask & (1u << i)) > 0 &&
					(memoryProperties.memoryTypes[i].propertyFlags & requiredMemoryFlags) == requiredMemoryFlags) {
					return i;
				}
			}
			throw std::runtime_error("No suitable memory type found");
		}

	}
}
#endif // !_DE_UTILITY_