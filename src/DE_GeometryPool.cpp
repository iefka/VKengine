#include"DE_Device.hpp"
#include "DE_GeometryPool.hpp"

#include<algorithm>

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
			uint32_t initialVertexCapacity,
			uint32_t initialIndexCapacity)
			:device_{device}
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
				device_.executeSingleCommand([&](vk::CommandBuffer cmd) {
					const auto region = vk::BufferCopy{}
						.setSrcOffset(0)
						.setDstOffset(0)
						.setSize(oldBytes);
					cmd.copyBuffer(*vertexBuffer_->getBuffer(), *newBuffer->getBuffer(), region);
				});
			}

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
				device_.executeSingleCommand([&](vk::CommandBuffer cmd) {
					const auto region = vk::BufferCopy{}
						.setSrcOffset(0)
						.setDstOffset(0)
						.setSize(oldBytes);
					cmd.copyBuffer(*indexBuffer_->getBuffer(), *newBuffer->getBuffer(), region);
				});
			}

			indexBuffer_ = std::move(newBuffer);
			indexAllocator_.grow(newCapacity);
		}
	
		void GeometryPool::uploadIndexData(uint32_t indexOffset,
			const std::vector<uint32_t>& indices) {
			if (indices.empty()) { return; }

			const vk::DeviceSize bufferSize = sizeof(uint32_t) * indices.size();
			MemoryUsageInfo stagingInfo{
				bufferSize,1,0,
				vk::BufferUsageFlagBits::eTransferSrc
			};
			Buffer staging(device_, stagingInfo);
			staging.map();
			staging.copyToBuffer(indices, 0);

			device_.executeSingleCommand([&](vk::CommandBuffer cmd) {
				auto region = vk::BufferCopy{}
					.setSize(bufferSize)
					.setSrcOffset(0)
					.setDstOffset(static_cast<vk::DeviceSize>(indexOffset * sizeof(uint32_t)));
				cmd.copyBuffer(*staging.getBuffer(), *indexBuffer_->getBuffer(), region);
			});

		}

		GeometryHandle GeometryPool::upload(const std::vector<Mesh::Vertex>& vertices,
			const std::vector<uint32_t>& indices) {

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
				const vk::DeviceSize bufferSize = sizeof(Mesh::Vertex) * vertexCount;
				MemoryUsageInfo stagingInfo{ bufferSize,1,0,vk::BufferUsageFlagBits::eTransferSrc };
				Buffer staging(device_, stagingInfo);
				staging.map();
				staging.copyToBuffer(vertices, 0);

				device_.executeSingleCommand([&](vk::CommandBuffer cmd) {

					const auto region = vk::BufferCopy{}
						.setSrcOffset(0)
						.setDstOffset(static_cast<vk::DeviceSize>(vAlloc->offset) * sizeof(Mesh::Vertex))
						.setSize(bufferSize);
					cmd.copyBuffer(*staging.getBuffer(), *vertexBuffer_->getBuffer(), region);
				});
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
			const std::vector<uint32_t>& indices) {
			
			const auto indexCount = static_cast<uint32_t>(indices.size());

			auto vAlloc = vertexAllocator_.allocate(vertexCount);
			if (!vAlloc) {
				growVertexBuffer(vertexCount);
				vAlloc = vertexAllocator_.allocate(vertexCount);
			}

			auto iAlloc = indexAllocator_.allocate(indexCount);
			if (!iAlloc) {
				growIndexBuffer(indexCount);
				iAlloc = vertexAllocator_.allocate(indexCount);
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
