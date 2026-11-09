
#include "DE_Instance.hpp"
#ifndef NDEBUG
#include"Utility/DE_Debug.hpp"
#endif 

namespace de{

	Instance::Instance(const char* appName, uint32_t applicationVersion,
		const char* engineName, uint32_t engineVersion,
		const std::vector<const char*>& extensions,
		const std::vector<const char*>& layers){

		
		std::vector<const char*> finalExtensions = extensions;
		std::vector<const char*> finalLayers = layers;

	#ifndef NDEBUG
		finalLayers.emplace_back("VK_LAYER_KHRONOS_validation");
			
		std::vector<const char*> debugExtensions{
			VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
		};
	#if defined(DE_VK_HARD_VALIDATION) && DE_VK_HARD_VALIDATION
    	debugExtensions.emplace_back(VK_EXT_VALIDATION_FEATURES_EXTENSION_NAME);
	#endif
		
		finalExtensions.insert(finalExtensions.end(), debugExtensions.begin(), debugExtensions.end());
	#endif 

		const auto appInfo = vk::ApplicationInfo{}
			.setPApplicationName(appName)
			.setApplicationVersion(applicationVersion)
			.setPEngineName(engineName)
			.setEngineVersion(engineVersion)
			.setApiVersion(vk::makeApiVersion(0,1,4,0));
			
	
		auto instanceInfo = vk::InstanceCreateInfo{}
			.setPApplicationInfo(&appInfo)
			.setEnabledExtensionCount(finalExtensions.size())
			.setPpEnabledExtensionNames(finalExtensions.data())
			.setEnabledLayerCount(finalLayers.size())
			.setPpEnabledLayerNames(finalLayers.data());


	#ifndef NDEBUG

		const auto debugMessengerInfo = vk::DebugUtilsMessengerCreateInfoEXT{}
			.setMessageSeverity(
				vk::DebugUtilsMessageSeverityFlagBitsEXT::eError
				|vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning
			)
			.setMessageType(
				vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral
				|vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation
				|vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance
			)
			.setPfnUserCallback(de::debug::MyDebugCallback);

		#if defined(DE_VK_HARD_VALIDATION) && DE_VK_HARD_VALIDATION
			const std::vector<vk::ValidationFeatureEnableEXT> enabledFeatures{
				vk::ValidationFeatureEnableEXT::eSynchronizationValidation,
				vk::ValidationFeatureEnableEXT::eGpuAssisted
			};

			auto validationFeaturesInfo = vk::ValidationFeaturesEXT{}
				.setEnabledValidationFeatures(enabledFeatures)
				.setPNext(&debugMessengerInfo);

			instanceInfo.pNext = &validationFeaturesInfo;
		#else
    		instanceInfo.pNext = &debugMessengerInfo;
		#endif
	#endif
		uInstance = vk::createInstanceUnique(instanceInfo);
	}
}