#pragma once
#ifndef _DE_UTILITY_
#define _DE_UTILITY_


#include<vulkan/vulkan.hpp>
#include <functional>
namespace de {
	namespace utl {

		inline uint32_t getFormatSize(vk::Format format) {
			switch (format)
			{
			case vk::Format::eR8G8B8A8Unorm:
				return static_cast<uint32_t>(4);
			case vk::Format::eR32G32B32A32Sfloat:
				return static_cast<uint32_t> (4 * sizeof(float));
			case vk::Format::eR32G32B32Sfloat:
				return static_cast<uint32_t> (3 * sizeof(float));

			default:
				throw std::invalid_argument("unsuported format");
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

		template <typename T, typename... Rest>
		inline void hashCombine(std::size_t& seed, const T& v, const Rest&... rest) {
			seed ^= std::hash<T>{}(v)+0x9e3779b9 + (seed << 6) + (seed >> 2);
			(hashCombine(seed, rest), ...);
		}

		inline vk::UniqueImageView createImageView(
			const vk::Device& logicalDevice,
			const vk::Image& image,
			const vk::Format& format,
			vk::ImageAspectFlagBits imageAspect) {


			const auto subresourceRange = vk::ImageSubresourceRange{}
				.setAspectMask(imageAspect)
				.setBaseMipLevel(0)
				.setLevelCount(1)
				.setBaseArrayLayer(0)
				.setLayerCount(1);

			const auto imageInfo = vk::ImageViewCreateInfo{}
				.setImage(image)
				.setViewType(vk::ImageViewType::e2D)
				.setFormat(format)
				.setSubresourceRange(subresourceRange);

			return logicalDevice.createImageViewUnique(imageInfo);
		}

	}
}
#endif // !_DE_UTILITY_