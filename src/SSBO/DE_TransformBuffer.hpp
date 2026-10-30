#pragma once

#include<vulkan/vulkan.hpp>
#include<vector>
#include<memory>
#include<cstdint>

#include"DE_Memory.hpp"
#include"DE_Descriptors.hpp"
#include"DE_TransformData.hpp"

namespace de {
	class Device;

	class TransformBuffer {
	public:
		TransformBuffer(const Device& device, uint32_t framesInFlight, uint32_t initialCapacity = 1024);
		void upload(uint32_t frameIndex, const std::vector<GPUTransformData>& transforms);

		vk::DescriptorSet getDescriptorSet(uint32_t frameIndex) const noexcept {
			return slots_[frameIndex].descriptorSet;
		}
		const DescriptorSetLayout& getDescriptorLayout()const noexcept{
			return *layout_;
		}

	private:
		struct FrameSlot {
			std::unique_ptr<Buffer> buffer;
			uint32_t capacity = 0;
			vk::DescriptorSet descriptorSet;
		};

		const Device& device_;

		void growSlot(FrameSlot& slot,uint32_t requiedCapacity);
		void rewriteDescriptor(FrameSlot& slot);

		std::unique_ptr<DescriptorPool> pool_;
		std::unique_ptr<DescriptorSetLayout> layout_;
		std::vector<FrameSlot> slots_;
	};

}