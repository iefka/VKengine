#include "DE_MaterialBuffer.hpp"
#include "DE_Device.hpp"
#include"gpu/DE_UploadContext.hpp"

#include<span>
#include<cstddef>



namespace de{

	namespace {
		MemoryUsageInfo makeMaterialBuffer(vk::DeviceSize sizeBytes) {
			return MemoryUsageInfo{
				sizeBytes,
				1,
				0,
				vk::BufferUsageFlagBits::eStorageBuffer
				| vk::BufferUsageFlagBits::eTransferSrc
				| vk::BufferUsageFlagBits::eTransferDst,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			};
		}
	}

	MaterialBuffer::MaterialBuffer(const Device& device, uint32_t initialCapacity) 
		:device_{ device }
		,allocator_{ initialCapacity } {

		buffer_ = std::make_unique<Buffer>(device_,
			makeMaterialBuffer(initialCapacity * sizeof(GPUMaterialData))
			);

		pool_ = std::make_unique<DescriptorPool>(
			DescriptorPool::Builder{ device_ }
			.setMaxSets(1)
			.addPoolSize(vk::DescriptorType::eStorageBuffer, 1)
			.build()
		);
		layout_ = std::make_unique<DescriptorSetLayout>(
			DescriptorSetLayout::Builder{ device_ }
			.addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eFragment)
			.build()
		);

		auto descriptorInfo = buffer_->getDescriptorInfo();
		descriptorSet_ = DescriptorWriter{ device_,*pool_,*layout_ }
			.writeBuffer(0, &descriptorInfo)
			.build();
	
	}
	uint32_t MaterialBuffer::upload(UploadContext& upload, const GPUMaterialData& data){
		auto alloc = allocator_.allocate(1);
		if (!alloc) {
			grow(upload,1);
			alloc = allocator_.allocate(1);
		}

		upload.copyBuffer(*buffer_->getBuffer(),
		std::as_bytes(std::span{&data,1}),
		static_cast<vk::DeviceSize>(alloc->offset) * sizeof(GPUMaterialData));
		
		return alloc->offset;
	}
	void MaterialBuffer::free(uint32_t index){
		allocator_.free(
			RangeAllocator::Allocation{ 
				.offset = index,
				.count = 1 
			}
		);
	}
	void MaterialBuffer::grow(UploadContext& upload,uint32_t additionalCount){
		const uint32_t oldCapacity = allocator_.getCapacity();
		const uint32_t newCapacity = std::max(oldCapacity * 2, oldCapacity + additionalCount);

		auto newBuffer = std::make_unique<Buffer>(device_,
			makeMaterialBuffer(newCapacity * sizeof(GPUMaterialData))
		);

		const vk::DeviceSize oldBytes = sizeof(GPUMaterialData) * oldCapacity;
		if (oldBytes > 0) {
				upload.cmd().copyBuffer(*buffer_->getBuffer(), *newBuffer->getBuffer(), 
					vk::BufferCopy{}.setSize(oldBytes));
			
		}

		buffer_ = std::move(newBuffer);
		allocator_.grow(newCapacity);
		//buffer changed, need to upate descriptor with new one
		rewriteDescriptor();
	}
	void MaterialBuffer::rewriteDescriptor(){
		auto bufferInfo = buffer_->getDescriptorInfo();
		DescriptorWriter{ device_,*pool_,*layout_ }
			.writeBuffer(0, &bufferInfo)
			.overwrite(descriptorSet_);
	}
}
