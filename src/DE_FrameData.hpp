#include<vulkan/vulkan.hpp>
#include"DE_GameObject.hpp"
#include "DE_Camera.hpp"
namespace de {
	
	#define MAX_LIGHTS 10

	struct pointLight {
		glm::vec4 lightPosition{ 1.f }; //ignore w 
		glm::vec4 lightColor{ 1.f,1.f,1.f,0.2f };//w intensity
		float radius{ 0.05f };
		float _padding[3]{};
	};

	struct GlobalUBO {
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverceViewMatrix{ 1.f };
		glm::vec4 ambientLightColor{ 1.f,1.f,1.f, .02f };
		pointLight pointLights[MAX_LIGHTS];
		int numLights;
	};

	struct FrameInfo
	{
		std::uint32_t frameIndex;
		vk::CommandBuffer commandBuffer;
		vk::DescriptorSet descriptorSet;
		const de::Camera& camera;
		const GameObject::Map& gameObjects;
	};
}