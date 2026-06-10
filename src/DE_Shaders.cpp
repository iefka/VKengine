#include "DE_Shaders.hpp"
#include"DE_Device.hpp"
//std
//!возможно нужно будет перенести в хедер если будут ошибки!
#include<fstream>
#include<exception>
#include<iostream>

namespace de{
	Shader::Shader(const Device& device, const std::filesystem::path& path){

		const auto& logicalDevice = device.getLogicalDevice();	
		
		std::ifstream file{path,std::ios::binary};
		if (!file.is_open()) {
			throw std::runtime_error("could not open file: " + path.string());
		}

		std::vector<uint32_t> buffer{};
		const auto fileSizeInBytes = std::filesystem::file_size(path);
		//divide because file_size return size in bytes
		buffer.resize(std::filesystem::file_size(path) / sizeof(uint32_t));

		file.seekg(0);
		file.read(reinterpret_cast<char*>(buffer.data()), fileSizeInBytes);

		const auto shaderModuleInfo = vk::ShaderModuleCreateInfo{}
		.setCode(buffer);

		shaderModule_ = logicalDevice.createShaderModuleUnique(shaderModuleInfo);
	}

	vk::PipelineShaderStageCreateInfo Shader::getStageInfo(vk::ShaderStageFlagBits states, const char* entryName) const{
		 
		return  vk::PipelineShaderStageCreateInfo{}
			.setModule(*shaderModule_)
			.setStage(states)
			.setPName("main");
	}
}
