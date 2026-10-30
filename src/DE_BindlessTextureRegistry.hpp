#pragma once

#include<vulkan/vulkan.hpp>

#include<cstdint>
#include<memory>

#include"DE_Descriptors.hpp"
#include"DE_ITextureSource.hpp"

namespace de {
	class Device;


	class BindlessTextureRegistry final{
	public:
		explicit BindlessTextureRegistry(const Device& device, uint32_t maxTextures);

		uint32_t registerTexture(const ITextureSource& texture);

		vk::DescriptorSetLayout getDescriptorLayout() const noexcept { return *layout_; }
		vk::DescriptorSet getDescriptorSet() const noexcept { return descriptorSet_; }
	private:
		const Device& device_;
		uint32_t maxTextures_;
		uint32_t nextFreeIndex_ = 0;

		vk::UniqueDescriptorSetLayout layout_;
		vk::UniqueDescriptorPool pool_;
		vk::DescriptorSet descriptorSet_;

	};
}