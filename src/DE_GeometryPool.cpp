#include "DE_GeometryPool.hpp"
#include"DE_Device.hpp"
#include"gpu/DE_UploadContext.hpp"

#include<algorithm>
#include<cstddef>

namespace de {

	namespace {
		MemoryUsageInfo makeVertexBufferInfo(vk::DeviceSize sizeBytes) {
			return MemoryUsageInfo{
				sizeBytes,1,0,
				vk::BufferUsageFlagBits::eVertexBuffer
				| vk::BufferUsageFlagBits::eStorageBuffer
				| vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			};
		}

		MemoryUsageInfo makeIndexBufferInfo(vk::DeviceSize sizeBytes) {
			return MemoryUsageInfo{
				sizeBytes,1,0,
				vk::BufferUsageFlagBits::eIndexBuffer
				| vk::BufferUsageFlagBits::eStorageBuffer
				| vk::BufferUsageFlagBits::eTransferDst
				| vk::BufferUsageFlagBits::eTransferSrc,
				vk::MemoryPropertyFlagBits::eDeviceLocal
			};
		}
	}
		GeometryPool::GeometryPool(
			const Device& device,
			UploadContext& upload,
			uint32_t initialVertexCapacity,
			uint32_t initialIndexCapacity)
			:device_{device}
			,upload_{upload}
			,vertexAllocator_{initialVertexCapacity}
			,indexAllocator_{initialIndexCapacity} {

			vertexBuffer_ = std::make_unique<Buffer>(device_,
				makeVertexBufferInfo(sizeof(Mesh::Vertex) * initialVertexCapacity));

			indexBuffer_ = std::make_unique<Buffer>(device_,
				makeIndexBufferInfo(sizeof(uint32_t) * initialIndexCapacity));
		}

		void GeometryPool::growVertexBuffer(uint32_t additionalCount) {
			const uint32_t oldCapacity = vertexAllocator_.getCapacity();
			const uint32_t newCapacity = std::max(oldCapacity * 2, oldCapacity + additionalCount);

			auto newBuffer = std::make_unique<Buffer>(device_,
				makeVertexBufferInfo(sizeof(Mesh::Vertex) * newCapacity));

			const vk::DeviceSize oldBytes = sizeof(Mesh::Vertex) * oldCapacity;

			if (oldBytes > 0) {
				upload_.cmd().copyBuffer(*vertexBuffer_->getBuffer(),*newBuffer->getBuffer(),
					vk::BufferCopy{}.setSize(oldBytes));
			}

			upload_.retire(std::move(vertexBuffer_));
			vertexBuffer_ = std::move(newBuffer);
			vertexAllocator_.grow(newCapacity);
		}

		void GeometryPool::growIndexBuffer(uint32_t additionalCount) {
			const uint32_t oldCapacity = indexAllocator_.getCapacity();
			const uint32_t newCapacity = std::max(oldCapacity * 2, oldCapacity + additionalCount);

			auto newBuffer = std::make_unique<Buffer>(device_,
				makeIndexBufferInfo(sizeof(uint32_t) * newCapacity));

			const vk::DeviceSize oldBytes = sizeof(uint32_t) * oldCapacity;

			if (oldBytes > 0) {
				
				upload_.cmd().copyBuffer(*indexBuffer_->getBuffer(),*newBuffer->getBuffer(),
					vk::BufferCopy{}.setSize(oldBytes));
			}

			upload_.retire(std::move(indexBuffer_));   
    		indexBuffer_ = std::move(newBuffer);
    		indexAllocator_.grow(newCapacity);
		}
	
		void GeometryPool::uploadIndexData(uint32_t indexOffset,
			std::span<const uint32_t> indices) {
			if (indices.empty()) { return; }

			upload_.copyBuffer(*indexBuffer_->getBuffer(),std::as_bytes(indices),
				static_cast<vk::DeviceSize>(indexOffset * sizeof(uint32_t)));
		}

		GeometryHandle GeometryPool::upload(std::span<const Mesh::Vertex> vertices,
			std::span<const uint32_t> indices) {

			const auto vertexCount = static_cast<uint32_t>(vertices.size());
			const auto indexCount = static_cast<uint32_t>(indices.size());

			auto vAlloc = vertexAllocator_.allocate(vertexCount);
			if (!vAlloc) {
				growVertexBuffer(vertexCount);
				vAlloc = vertexAllocator_.allocate(vertexCount);
			}

			auto iAlloc = indexAllocator_.allocate(indexCount);
			if (!iAlloc) {
				growIndexBuffer(indexCount);
				iAlloc = indexAllocator_.allocate(indexCount);
			}

			if (vertexCount > 0) {
		
				upload_.copyBuffer(*vertexBuffer_->getBuffer(),std::as_bytes(vertices),
					static_cast<vk::DeviceSize>(vAlloc->offset) * sizeof(Mesh::Vertex));
			}

			uploadIndexData(iAlloc->offset, indices);

			return GeometryHandle{
				.vertexOffset = vAlloc->offset,
				.vertexCount = vAlloc->count,
				.indexOffset = iAlloc->offset,
				.indexCount = iAlloc->count
			};
		}

		GeometryHandle GeometryPool::reserveForCompute(uint32_t vertexCount,
			std::span<const uint32_t> indices) {
			
			const auto indexCount = static_cast<uint32_t>(indices.size());

			auto vAlloc = vertexAllocator_.allocate(vertexCount);
			if (!vAlloc) {
				growVertexBuffer(vertexCount);
				vAlloc = vertexAllocator_.allocate(vertexCount);
			}

			auto iAlloc = indexAllocator_.allocate(indexCount);
			if (!iAlloc) {
				growIndexBuffer(indexCount);
				iAlloc = indexAllocator_.allocate(indexCount);
			}

			uploadIndexData(iAlloc->offset, indices);
			
			return GeometryHandle{
				.vertexOffset = vAlloc->offset,
				.vertexCount = vAlloc->count,
				.indexOffset = iAlloc->offset,
				.indexCount = iAlloc->count
			};
		}

		vk::DescriptorBufferInfo GeometryPool::getVertexRangeDescriptor(
			uint32_t vertexOffset,
			uint32_t vertexCount) const noexcept {
			return vk::DescriptorBufferInfo{}
				.setBuffer(*vertexBuffer_->getBuffer())
				.setOffset(static_cast<vk::DeviceSize>(vertexOffset) * sizeof(Mesh::Vertex))
				.setRange(static_cast<vk::DeviceSize>(vertexCount) * sizeof(Mesh::Vertex));
		}

		vk::Buffer GeometryPool::getVertexBufferHandle() const noexcept {
			return *vertexBuffer_->getBuffer();
		}

		void GeometryPool::free(const GeometryHandle& handle) {
			vertexAllocator_.free(RangeAllocator::Allocation{
				.offset = handle.vertexOffset,
				.count = handle.vertexCount
			});
			indexAllocator_.free(RangeAllocator::Allocation{
				.offset = handle.indexOffset,
				.count = handle.indexCount
			});
		}

		void GeometryPool::bind(const vk::CommandBuffer& commandBuffer) const {
			commandBuffer.bindVertexBuffers(0, *vertexBuffer_->getBuffer(), { 0 });
			commandBuffer.bindIndexBuffer(*indexBuffer_->getBuffer(), 0,vk::IndexType::eUint32);
		}
		
}
