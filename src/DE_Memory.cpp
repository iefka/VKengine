#include "DE_Memory.hpp"
#include"DE_Device.hpp"
#include"Utility/DE_Utility.hpp"
//std
#include<memory>
#include<algorithm>

namespace de{

	vk::DeviceSize alingUp(vk::DeviceSize value, vk::DeviceSize aligment) {
		if (aligment > 0) {
			return (value + aligment - 1) & ~(aligment - 1);
		}
		return value;
	}

	Buffer::Buffer(const Device& device,
		const MemoryUsageInfo& memoryInfo) : physicalDevice_(device.getPhysicalDevice()),
			logicalDevice_(device.getLogicalDevice()){

		const auto bufferCreateInfo = vk::BufferCreateInfo{}
			.setSize(memoryInfo.size)
			.setUsage(memoryInfo.bufferUsage)
			.setSharingMode(vk::SharingMode::eExclusive);
		gpuBuffer_.buffer = logicalDevice_.createBufferUnique(bufferCreateInfo);


		memoryRequirements_ = logicalDevice_.getBufferMemoryRequirements(*gpuBuffer_.buffer);
		const auto memoryProp = physicalDevice_.getMemoryProperties();

		const auto aligmentSize = alingUp(memoryInfo.size, memoryRequirements_.alignment);

		bufferSize_ = memoryInfo.size;
		step_ = aligmentSize;
		aligmentBuferSize_ = + aligmentSize * memoryInfo.instanceCount;

		const auto memoryTypeIndex = utl::getSuitableMemoryIndex(memoryProp,
			memoryRequirements_.memoryTypeBits, memoryInfo.memoryFlags);

		const auto allocateInfo = vk::MemoryAllocateInfo{}
			.setAllocationSize(aligmentBuferSize_)
			.setMemoryTypeIndex(memoryTypeIndex);

		gpuBuffer_.memory = logicalDevice_.allocateMemoryUnique(allocateInfo);

		logicalDevice_.bindBufferMemory(*gpuBuffer_.buffer, *gpuBuffer_.memory, memoryInfo.offset);
	}

	Buffer::~Buffer(){
		unmap();
	}

	void Buffer::map(vk::DeviceSize size, vk::DeviceSize offset){
		mapedMemory_ = logicalDevice_.mapMemory(*gpuBuffer_.memory, offset, size == vk::WholeSize? aligmentBuferSize_ : size);
	}

	void Buffer::unmap(){
		if (mapedMemory_) {
			logicalDevice_.unmapMemory(*gpuBuffer_.memory);
		}
		mapedMemory_ = nullptr;
	}
	void Buffer::flush(vk::DeviceSize size, vk::DeviceSize offset){

		vk::DeviceSize flushSize = (size == vk::WholeSize) ? aligmentBuferSize_ - offset : size;
		const auto memoryRange = vk::MappedMemoryRange{}
			.setMemory(*gpuBuffer_.memory)
			.setSize(flushSize)
			.setOffset(offset);
		logicalDevice_.flushMappedMemoryRanges(memoryRange);
	}
	vk::DescriptorBufferInfo Buffer::getDescriptorInfo(vk::DeviceSize offset) const
	{
		return vk::DescriptorBufferInfo()
			.setBuffer(*gpuBuffer_.buffer)
			.setRange(bufferSize_)
			.setOffset(offset);
	}
	void Buffer::copyToBuffer(const void* data, vk::DeviceSize size, vk::DeviceSize offset) {
		if (!mapedMemory_) {
			throw std::runtime_error("cannot copy to buffer: unmaped buffer\n");
		}
		if (!((offset + size) <= aligmentBuferSize_)) {
			throw std::runtime_error("cannot copy to buffer: out of buffer size");
		}
		char* dst = static_cast<char*>(mapedMemory_) + offset;
		memcpy(dst, data, size);
	}

	//-image implementation-
	Image::Image(const Device& device, const ImageUsageInfo& imageInfo) : device_{device}{
	
		const auto& logicalDevice = device_.getLogicalDevice();
		const auto& physicalDevice = device_.getPhysicalDevice();

			gpuImage_.image = logicalDevice.createImageUnique(imageInfo.createInfo);


			const auto memoryRequirements_ = logicalDevice.getImageMemoryRequirements(*gpuImage_.image);
			const auto memoryProp = physicalDevice.getMemoryProperties();

			const auto aligmentSize = alingUp(memoryRequirements_.size, memoryRequirements_.alignment);

			const auto memoryTypeIndex = utl::getSuitableMemoryIndex(memoryProp,
				memoryRequirements_.memoryTypeBits, imageInfo.memoryFlags);

			const auto allocateInfo = vk::MemoryAllocateInfo{}
				.setAllocationSize(aligmentSize)
				.setMemoryTypeIndex(memoryTypeIndex);

			gpuImage_.memory = logicalDevice.allocateMemoryUnique(allocateInfo);

			logicalDevice.bindImageMemory(*gpuImage_.image, *gpuImage_.memory, imageInfo.offset);
	}
}


