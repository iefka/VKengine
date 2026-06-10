#include "DE_Device.hpp"
#include "DE_CommandBuffers.hpp"

namespace de {
	CommandPool::CommandPool(const Device& device, uint32_t queueFamilyIndex, 
		vk::CommandPoolCreateFlagBits flags):device_(device){

		const auto& logicalDevice = device_.getLogicalDevice();

		const auto commandPoolInfo = vk::CommandPoolCreateInfo{}
			.setQueueFamilyIndex(queueFamilyIndex)
			.setFlags(flags);

		pool_ = logicalDevice.createCommandPoolUnique(commandPoolInfo);
	}
	CommandBuffer CommandPool::allocate  (vk::CommandBufferLevel bufferLevel, uint32_t bufferCount){
		
		const auto& logicalDevice = device_.getLogicalDevice();
		
		const auto cmdBufferAllocateInfo = vk::CommandBufferAllocateInfo{}
			.setLevel(bufferLevel)
			.setCommandBufferCount(bufferCount)
			.setCommandPool(*pool_);

		return CommandBuffer(logicalDevice.allocateCommandBuffersUnique(cmdBufferAllocateInfo));
	}
	
}

