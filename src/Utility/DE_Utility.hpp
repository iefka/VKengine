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
			case vk::Format::eR8G8B8A8Srgb:
				return static_cast<uint32_t>(4);
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
			vk::ImageAspectFlagBits imageAspect,
			vk::ImageViewType viewType = vk::ImageViewType::e2D,
			uint32_t layerCount = 1,
			uint32_t baseArrayLayer = 0,
			uint32_t mipLevels = 1,
			uint32_t baseMipLevel = 0) {

			const auto subresourceRange = vk::ImageSubresourceRange{}
				.setAspectMask(imageAspect)
				.setBaseMipLevel(baseMipLevel)
				.setLevelCount(mipLevels)
				.setBaseArrayLayer(baseArrayLayer)
				.setLayerCount(layerCount);

			const auto imageInfo = vk::ImageViewCreateInfo{}
				.setImage(image)
				.setViewType(viewType)
				.setFormat(format)
				.setSubresourceRange(subresourceRange);

			return logicalDevice.createImageViewUnique(imageInfo);
		}

		inline vk::UniqueSampler createTextureSampler(const vk::Device& logicalDevice,
			vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eRepeat,
			bool anisotropyEnable = true,
			float maxAnisotropy = 4.0f) {

			const auto samplerInfo = vk::SamplerCreateInfo{}
				.setAddressModeU(addressMode)
				.setAddressModeV(addressMode)
				.setAddressModeW(addressMode)
				.setMagFilter(vk::Filter::eLinear)
				.setMinFilter(vk::Filter::eLinear)
				.setAnisotropyEnable(anisotropyEnable)
				.setMaxAnisotropy(maxAnisotropy)
				.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
				.setUnnormalizedCoordinates(vk::False)
				.setCompareEnable(vk::False)
				.setCompareOp(vk::CompareOp::eAlways)
				.setMipmapMode(vk::SamplerMipmapMode::eLinear)
				.setMipLodBias(0.f)
				.setMinLod(0.f)
				.setMaxLod(0.f);

			return logicalDevice.createSamplerUnique(samplerInfo);
		}
	}
}
#endif // !_DE_UTILITY_