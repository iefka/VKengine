#ifndef _DE_MEMORY_
#define _DE_MEMORY_


#include<vulkan/vulkan.hpp>


namespace de{
	class Device;


	struct ImageUsageInfo {


		ImageUsageInfo(vk::DeviceSize size_ = 0,
		vk::DeviceSize offset_ = 0,
		vk::ImageCreateInfo createInfo_ = {},
		vk::MemoryPropertyFlags memoryFlags_ = { vk::MemoryPropertyFlagBits::eDeviceLocal })
			:size{size_},
			offset{offset_},
			createInfo{createInfo_},
			memoryFlags{memoryFlags_}{}


		vk::DeviceSize size = 0;
		vk::DeviceSize offset = 0;
		vk::ImageCreateInfo createInfo = {};
		vk::MemoryPropertyFlags memoryFlags = { vk::MemoryPropertyFlagBits::eDeviceLocal };
	};

	struct MemoryUsageInfo{

		MemoryUsageInfo(vk::DeviceSize size_ = {0},
			uint32_t instanceCount_ = 1,
			vk::DeviceSize offset_ = {0},
			vk::BufferUsageFlags bufferUsage_ = {},
			vk::MemoryPropertyFlags memoryFlags_ = {vk::MemoryPropertyFlagBits::eHostVisible
			| vk::MemoryPropertyFlagBits::eHostCoherent})
				:size{size_},
				instanceCount{instanceCount_},
				bufferUsage{bufferUsage_},
				memoryFlags{memoryFlags_},
				offset{offset_} {}

		vk::DeviceSize			size		  = 0;
		uint32_t				instanceCount = 1;
		vk::BufferUsageFlags	bufferUsage = {};
		vk::DeviceSize			offset = 0;
		vk::MemoryPropertyFlags memoryFlags{
										vk::MemoryPropertyFlagBits::eHostVisible
										| vk::MemoryPropertyFlagBits::eHostCoherent };
	};
	class Buffer{
	public:
		Buffer(const Device& device,
			const MemoryUsageInfo& memoryInfo);
		~Buffer();

		Buffer(const Buffer&) = delete;
		Buffer& operator=(const Buffer&) = delete;

		Buffer(Buffer&&) = default;
		Buffer& operator=(Buffer&&) = default;

		const vk::UniqueBuffer& getBuffer() const noexcept{
			return gpuBuffer_.buffer;
		}

		//save maped memory into class (memory automaticly unmap when object destruct)
		void map(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize = 0);
		void unmap();
		void flush(vk::DeviceSize size = vk::WholeSize, vk::DeviceSize offset = 0);
		vk::DescriptorBufferInfo getDescriptorInfo(vk::DeviceSize offset = 0) const;

		//copy data into maped memory
		void copyToBuffer(const void* data, vk::DeviceSize size, vk::DeviceSize offset);
		
		template <typename Container>
		void copyToBuffer(const Container& cont, vk::DeviceSize offset = 0) {
			using ValueType = typename Container::value_type;
			copyToBuffer(cont.data(), cont.size() * sizeof(ValueType), offset);
		}
	private:

		const vk::PhysicalDevice& physicalDevice_;
		const vk::Device& logicalDevice_;

		vk::MemoryRequirements memoryRequirements_;
		vk::DeviceSize step_;
		vk::DeviceSize aligmentBuferSize_;
		vk::DeviceSize bufferSize_;
		void* mapedMemory_ = nullptr;
		struct{
			vk::UniqueBuffer buffer;
			vk::UniqueDeviceMemory memory;
		}gpuBuffer_;

		
	};

	class Image {
	public:
		Image(const Device& device,const ImageUsageInfo& imageInfo);
		~Image() = default;

		Image(const Image&) = delete;
		Image& operator=(const Image&) = delete;

		Image(Image&&) = default;
		Image& operator=(Image&&) = default;

		const vk::Image getImage() const noexcept {
			return *gpuImage_.image;
		}
	private:

		const Device& device_;

		struct {
			vk::UniqueImage image;
			vk::UniqueDeviceMemory memory;
		}gpuImage_;
	};
	
}
#endif // !_DE_MEMORY_