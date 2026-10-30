#include"DE_Device.hpp"
#include "DE_IndirectCommandBuffer.hpp"
#include <algorithm>

namespace de {

	namespace {
		MemoryUsageInfo createIndirectBufferInfo(vk::DeviceSize byteSize) {
			return MemoryUsageInfo{
				byteSize,1,0,
				vk::BufferUsageFlagBits::eIndirectBuffer,
				vk::MemoryPropertyFlagBits::eHostVisible
			};
		}
	}

	IndirectCommandBuffer::IndirectCommandBuffer(const Device& device, 
		uint32_t framesInFlight, uint32_t initialCapacity) :device_{ device } {
	
		slots_.resize(framesInFlight);
		for(auto& slot : slots_){
			slot.capacity = initialCapacity;
			slot.buffer = std::make_unique<Buffer>(device_,
				createIndirectBufferInfo(static_cast<vk::DeviceSize>(initialCapacity)
					* sizeof(vk::DrawIndexedIndirectCommand))
			);
			slot.buffer->map();
		}
	
	}
	void IndirectCommandBuffer::upload(uint32_t frameIndex, 
		const std::vector<vk::DrawIndexedIndirectCommand>& command){
		auto& slot = slots_[frameIndex];

		const uint32_t requied = static_cast<uint32_t>(command.size());

		if (requied > slot.capacity) {
			growSlot(slot, requied);
		}

		if(!command.empty()){
			slot.buffer->copyToBuffer(command, 0);
			slot.buffer->flush();
		}
	}
	vk::Buffer IndirectCommandBuffer::getBuffer(uint32_t frameIndex) const noexcept{
		return *slots_[frameIndex].buffer->getBuffer();
	}
	void IndirectCommandBuffer::growSlot(FrameSlot& slot, uint32_t requiedCapacity){
		const uint32_t newCapacity = std::max(slot.capacity * 2, requiedCapacity);

		slot.buffer = std::make_unique<Buffer>(device_,
			createIndirectBufferInfo(static_cast<vk::DeviceSize>(newCapacity)
				* sizeof(vk::DrawIndexedIndirectCommand))
		);
		slot.capacity = newCapacity;
		slot.buffer->map();
	}
}
