#ifndef _DE_MEMORY_
#define _DE_MEMORY_


#include<vulkan/vulkan.hpp>


namespace de{
	class Device;


	struct MemoryUsageInfo{

		MemoryUsageInfo(vk::DeviceSize size_ = {0},
			vk::DeviceSize offset_ = {0},
			vk::BufferUsageFlags bufferUsage_ = {},
			vk::MemoryPropertyFlags memoryFlags_ = {vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent})
				:size{size_},
				bufferUsage{bufferUsage_},
				memoryFlags{memoryFlags_},
				offset{offset_} {}

		vk::DeviceSize			size		= 0;
		vk::BufferUsageFlags	bufferUsage = {};
		vk::DeviceSize			offset		= 0;
		vk::MemoryPropertyFlags memoryFlags =
										vk::MemoryPropertyFlagBits::eHostVisible 
										| vk::MemoryPropertyFlagBits::eHostCoherent;
	};
	class Buffer{
	public:
		Buffer(const Device& device,
			const MemoryUsageInfo& memoryInfo);
		~Buffer() = default;

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		Buffer(Buffer&&) = default;
		Buffer& operator=(Buffer&&) = default;

		const vk::UniqueBuffer& getBuffer() const noexcept{
			return gpuBuffer_.buffer;
		}

		void* map();
		void unmap();

		template <typename Container>
		void copyToBuffer(const Container& cont) {

			void* mapedMemory = this->map();
			memcpy(mapedMemory, cont.data(), sizeof(cont));
			this->unmap();
		}

	private:

		const vk::PhysicalDevice& physicalDevice_;
		const vk::Device& logicalDevice_;

		vk::MemoryRequirements memoryRequirements_;
		vk::DeviceSize dataOffset_;

		struct{
			vk::UniqueBuffer buffer;
			vk::UniqueDeviceMemory memory;
		}gpuBuffer_;

		
	};

	
}
#endif // !_DE_MEMORY_