#include "DE_Memory.hpp"
#include"DE_Device.hpp"
#include"Utility/DE_Utility.hpp"
//std
#include<memory>
#include<algorithm>

namespace de{
	Buffer::Buffer(const Device& device,
		const MemoryUsageInfo& memoryInfo) : physicalDevice_(device.getPhysicalDevice()),
			logicalDevice_(device.getLogicalDevice()){

		dataOffset_ = memoryInfo.offset;

		const auto bufferCreateInfo = vk::BufferCreateInfo{}
			.setSize(memoryInfo.size)
			.setUsage(memoryInfo.bufferUsage)
			.setSharingMode(vk::SharingMode::eExclusive);
		gpuBuffer_.buffer = logicalDevice_.createBufferUnique(bufferCreateInfo);

		memoryRequirements_ = logicalDevice_.getBufferMemoryRequirements(*gpuBuffer_.buffer);
		const auto memoryProp = physicalDevice_.getMemoryProperties();

		const auto memoryTypeIndex = utl::getSuitableMemoryIndex(memoryProp,
			memoryRequirements_.memoryTypeBits, memoryInfo.memoryFlags);

		const auto allocateInfo = vk::MemoryAllocateInfo{}
			.setAllocationSize(memoryRequirements_.size)
			.setMemoryTypeIndex(memoryTypeIndex);

		gpuBuffer_.memory = logicalDevice_.allocateMemoryUnique(allocateInfo);

		logicalDevice_.bindBufferMemory(*gpuBuffer_.buffer, *gpuBuffer_.memory, memoryInfo.offset);
	}
	

	//return maped memory
	void* Buffer::map(){
		return logicalDevice_.mapMemory(*gpuBuffer_.memory, dataOffset_, memoryRequirements_.size);
	}

	void Buffer::unmap(){
		logicalDevice_.unmapMemory(*gpuBuffer_.memory);
	}

}


