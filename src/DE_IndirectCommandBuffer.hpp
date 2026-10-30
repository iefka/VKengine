#pragma once

#include<vulkan/vulkan.hpp>

#include<memory>
#include<cstdint>
#include<vector>

#include"DE_Memory.hpp"

namespace de {
	class Device;
	
	class IndirectCommandBuffer final {
	public:
		IndirectCommandBuffer(const Device& device,
			uint32_t framesInFlight, uint32_t initialCapacity = 1024);
		void upload(uint32_t frameIndex, const std::vector<vk::DrawIndexedIndirectCommand>& command);
		vk::Buffer getBuffer(uint32_t frameIndex) const noexcept;
	private:

		struct FrameSlot {
			std::unique_ptr<Buffer> buffer;
			uint32_t capacity = 0;
		};

		void growSlot(FrameSlot& slot, uint32_t requiedCapacity);

		const Device& device_;
		std::vector<FrameSlot> slots_;
	};
}