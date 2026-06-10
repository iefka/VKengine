#ifndef _DE_INSTANCE_
#define _DE_INSTANCE_

//vk
#include<vector>
#include<string>
#include"vulkan/vulkan.hpp"
//std
#include<exception>

namespace de{

	class Instance{
	public:

		Instance(const char* appName, uint32_t applicationVersion = 1u,
			const char* engineName = "none", uint32_t engineVersion = 1u,
			const std::vector<const char*>& extensions = {},
			const std::vector<const char*>& layers = {});
		~Instance() = default;

		Instance(const Instance&) = delete;
		Instance& operator=(const Instance&) = delete;

		Instance(Instance&&) = default;
		Instance& operator=(Instance&&) = default;

		 vk::Instance getInstance() const noexcept
		 {return uInstance.get();}

	private:
		vk::UniqueInstance uInstance;
	};

}
#endif // !_DE_INSTANCE_