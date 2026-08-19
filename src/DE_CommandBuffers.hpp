#ifndef _DE_COMMAND_BUFFERS_
#define _DE_COMMAND_BUFFERS_
#include <vulkan/vulkan.hpp>
#include<utility>

namespace de {
	class Device;

	class CommandBuffer {
	public:
		explicit CommandBuffer(std::vector<vk::UniqueCommandBuffer> commandBuffer) :cmdBuffer_(std::move(commandBuffer)) {}

		const vk::CommandBuffer handle(uint32_t bufferIndex) const noexcept { return *cmdBuffer_[bufferIndex]; }

	private:
		std::vector <vk::UniqueCommandBuffer> cmdBuffer_;
	};

	class CommandPool {
	public:
		CommandPool(const Device& device, uint32_t queueFamilyIndex, vk::CommandPoolCreateFlagBits flags = {});

		CommandBuffer allocate(vk::CommandBufferLevel bufferLevel, uint32_t bufferCount = 1) const;

	private:

		const Device& device_;
		vk::UniqueCommandPool pool_;
	};

}


#endif 
