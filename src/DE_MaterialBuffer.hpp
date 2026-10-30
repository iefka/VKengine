#pragma once

#include<vector>
#include<memory>
#include<cstdint>

#include"DE_Memory.hpp"
#include"DE_MaterialData.hpp"
#include"Utility/DE_RangeAllocator.hpp"
#include"DE_Descriptors.hpp"

namespace de {
	class Device;

	class MaterialBuffer {
	public:
		explicit MaterialBuffer(const Device& device, uint32_t initialCapacity);

		uint32_t upload(const GPUMaterialData& data);
		void free(uint32_t index);

		const DescriptorSetLayout& getDescriptorLayout() const noexcept { return *layout_; }
		vk::DescriptorSet getDescriptorSet() const noexcept { return descriptorSet_; }
	private:
		void grow(uint32_t additionalCount);
		void rewriteDescriptor();

		const Device& device_;

		std::unique_ptr<Buffer> buffer_;
		RangeAllocator allocator_;

		std::unique_ptr<DescriptorPool> pool_;
		std::unique_ptr<DescriptorSetLayout> layout_;
		vk::DescriptorSet descriptorSet_;
	};

}