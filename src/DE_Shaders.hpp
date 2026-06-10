#ifndef _DE_SHADERS_
#define _DE_SHADERS_

#include<vulkan/vulkan.hpp>
#include<filesystem>

namespace de{
	class Device;
	class Shader{
	public:
		Shader(const Device& device, const std::filesystem::path& path);
		~Shader() = default;

		Shader(const Shader&) = delete;
		Shader& operator=(const Shader&) = delete;

		Shader(Shader&&) = default;
		Shader& operator=(Shader&&) = default;


		const vk::ShaderModule& getShaderModule() const noexcept{ return *shaderModule_; }
		vk::PipelineShaderStageCreateInfo getStageInfo(vk::ShaderStageFlagBits states, 
const char* entryName = "main") const ;

	private:
		vk::UniqueShaderModule shaderModule_;
	};
}


#endif 

