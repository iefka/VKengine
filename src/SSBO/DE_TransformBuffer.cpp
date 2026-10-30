#include"DE_Device.hpp"
#include "DE_TransformBuffer.hpp"

#include<algorithm>
#include<ranges>

namespace de {

	namespace {
		MemoryUsageInfo makeTransformBufferInfo(vk::DeviceSize sizeBytes) {
			return MemoryUsageInfo{
				sizeBytes,1,0,
				vk::BufferUsageFlagBits::eStorageBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible
			};
		}

	}

	TransformBuffer::TransformBuffer(const Device& device,
		uint32_t framesInFlight, uint32_t initialCapacity)
		:device_{ device } {
	
		pool_ = std::make_unique<DescriptorPool>(
			DescriptorPool::Builder{ device_ }
				.setMaxSets(framesInFlight)
				.addPoolSize(vk::DescriptorType::eStorageBuffer, framesInFlight)
				.build()
		);

		layout_ = std::make_unique<DescriptorSetLayout>(
			DescriptorSetLayout::Builder{ device_ }
			.addBinding(0, vk::DescriptorType::eStorageBuffer,
				vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eVertex)
			.build()
		);

		slots_.resize(framesInFlight);
		for (auto& slot : slots_) {
			slot.capacity = initialCapacity;
			slot.buffer = std::make_unique<Buffer>(
				device, makeTransformBufferInfo(initialCapacity * sizeof(GPUTransformData))
			);
			slot.buffer->map();

			rewriteDescriptor(slot);
		}
	}
	void TransformBuffer::upload(uint32_t frameIndex, const std::vector<GPUTransformData>& transforms){
		auto& slot = slots_[frameIndex];

		const uint32_t requied = static_cast<uint32_t>(transforms.size());
		if (requied > slot.capacity) {
			growSlot(slot, requied);
		}
		if (!transforms.empty()) {
			slot.buffer->copyToBuffer(transforms, 0);
			slot.buffer->flush();
		}
	}
	void TransformBuffer::growSlot(FrameSlot& slot, uint32_t requiedCapacity){
		const uint32_t newCapacity = std::max(slot.capacity * 2, requiedCapacity);
		slot.buffer = std::make_unique<Buffer>(device_,
			makeTransformBufferInfo(
				static_cast<vk::DeviceSize>(newCapacity)
				* sizeof(GPUTransformData))
		);
		slot.buffer->map();
		slot.capacity = newCapacity;

		rewriteDescriptor(slot);
	}
	void TransformBuffer::rewriteDescriptor(FrameSlot & slot){
		auto bufferInfo = slot.buffer->getDescriptorInfo();

		if (!slot.descriptorSet) {
			slot.descriptorSet = DescriptorWriter{ device_,*pool_,*layout_ }
				.writeBuffer(0, &bufferInfo)
				.build();
		}
		else {
			DescriptorWriter{ device_,*pool_,*layout_ }
				.writeBuffer(0, &bufferInfo)
				.overwrite(slot.descriptorSet);
		}
	}
}
