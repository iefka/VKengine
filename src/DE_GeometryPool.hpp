#pragma once

#include<vulkan/vulkan.hpp>
#include<memory>
#include<vector>

#include"DE_Memory.hpp"
#include"Utility/DE_RangeAllocator.hpp"
#include"DE_GeometryHandle.hpp"
#include"Mesh/DE_Mesh.hpp"

namespace de {
	class Device;

	class GeometryPool {
	public:

		explicit GeometryPool(const Device& device,
			uint32_t initialVertexCapacity = 65536,
			uint32_t initialIndexCapacity = 262144);


		//load path for .obj files
		GeometryHandle upload(const std::vector<Mesh::Vertex>& vertices,
			const std::vector<uint32_t>& indices);

		//reserve range for compute geomety for further write
		GeometryHandle reserveForCompute(uint32_t vertexCount,
			const std::vector<uint32_t>& indices);


		//descriptor for compute shader
		vk::DescriptorBufferInfo getVertexRangeDescriptor(uint32_t vertexOffset,
			uint32_t vertexCount) const noexcept;

		//raw handle for memory barier after compute write
		vk::Buffer getVertexBufferHandle() const noexcept;

		void free(const GeometryHandle& handle);

		void bind(const vk::CommandBuffer& commandBuffer) const;

	private:

		void growVertexBuffer(uint32_t additionalCount);
		void growIndexBuffer(uint32_t additionalCount);

		void uploadIndexData(uint32_t indexOffset, const std::vector<uint32_t>& indices);

		const Device& device_;

		std::unique_ptr<Buffer> vertexBuffer_;
		RangeAllocator vertexAllocator_;

		std::unique_ptr<Buffer> indexBuffer_;
		RangeAllocator indexAllocator_;
	};
}