#define DEBUG
#include "DE_Instance.hpp"

namespace de{

	Instance::Instance(const char* appName, uint32_t applicationVersion,
		const char* engineName, uint32_t engineVersion,
		const std::vector<const char*>& extensions,
		const std::vector<const char*>& layers){

		
		std::vector<const char*> finalExtensions = extensions;
		std::vector<const char*> finalLayers = layers;

	#ifdef DEBUG
		finalLayers.emplace_back("VK_LAYER_KHRONOS_validation");
			
		const std::vector<const char*> debugExtensions{
			VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
			VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME
			
		};
		
		finalExtensions.insert(finalExtensions.end(), debugExtensions.begin(), debugExtensions.end());
	#endif 

		const auto appInfo = vk::ApplicationInfo{}
			.setPApplicationName(appName)
			.setApplicationVersion(applicationVersion)
			.setPEngineName(engineName)
			.setEngineVersion(engineVersion)
			.setApiVersion(vk::makeApiVersion(0,1,4,0));
			
		
		const auto instanceInfo = vk::InstanceCreateInfo{}
			.setPApplicationInfo(&appInfo)
			.setEnabledExtensionCount(finalExtensions.size())
			.setPpEnabledExtensionNames(finalExtensions.data())
			.setEnabledLayerCount(finalLayers.size())
			.setPpEnabledLayerNames(finalLayers.data());

		uInstance = vk::createInstanceUnique(instanceInfo);
	}
}