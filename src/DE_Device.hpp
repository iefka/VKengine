#ifndef _DE_DEVICE_
#define _DE_DEVICE_


//std
#include<vector>
#include<exception>
#include<optional>
//vk
#include<vulkan/vulkan.hpp>


struct QueueFamily{
	uint32_t queuFamilyIndex = UINT32_MAX;
	vk::Queue queue;
};

namespace de{

	class Instance;
	class Device{
	public:

		Device(Instance& instance,std::optional<vk::SurfaceKHR> surface = std::nullopt, const std::vector<const char*>& extensions = {},
			const std::vector<const char*>& layers = {});
		~Device() = default;

		Device(const Device&) = delete;
		Device& operator=(const Device&) = delete;

		Device(Device&&) = default;
		Device& operator=(Device&&) = default;

		const vk::PhysicalDevice& getPhysicalDevice() const noexcept{ return physicalDevice_; }
		const vk::Device& getLogicalDevice() const noexcept{ return *uLogicalDevice_; }
		const QueueFamily getGraphicsQueue() const noexcept { return queues_[0]; }
		const QueueFamily getPresentQueue() const noexcept { return queues_[1]; }

	private:

		const Instance& instance_;
		vk::PhysicalDevice physicalDevice_;
		vk::UniqueDevice uLogicalDevice_;
		std::vector<QueueFamily> queues_;

		const std::vector<const char*> extensions_;
		const std::vector<const char*> layers_;

		//return discrete gpu from vector of all user gpus
		vk::PhysicalDevice pickDiscreteGpu(const std::vector<vk::PhysicalDevice>& devices);

		uint32_t pickSuitableQueueFamily(const std::vector<vk::QueueFamilyProperties>& queueFamilies,
			vk::QueueFlags requiredFlags, std::optional<vk::SurfaceKHR> surface = std::nullopt);

		vk::UniqueDevice createLogicalDevice();
	};

}
#endif