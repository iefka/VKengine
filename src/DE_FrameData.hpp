#include<vulkan/vulkan.hpp>

#include <cstddef>

#include"DE_GameObject.hpp"
#include "DE_Camera.hpp"

namespace de {
	
	inline constexpr int kMaxLights = 10;

	struct pointLight {
		glm::vec4 lightPosition{ 1.f }; //ignore w 
		glm::vec4 lightColor{ 1.f,1.f,1.f,0.2f };//w intensity
		float radius{ 0.05f };
		float _padding[3];
	};

	struct GlobalUBO {
		glm::mat4 projectionMatrix{ 1.f };
		glm::mat4 viewMatrix{ 1.f };
		glm::mat4 inverceViewMatrix{ 1.f };
		glm::mat4 invViewProjMatrix{ 1.f };
		glm::vec4 ambientLightColor{ 1.f,1.f,1.f, .02f };
		pointLight pointLights[kMaxLights];
		int numLights;
		float _padding[3];
	};

	struct FrameInfo
	{
		std::uint32_t frameIndex;
		vk::CommandBuffer commandBuffer;
		vk::DescriptorSet descriptorSet;
		const de::Camera& camera;
		const GameObject::Map& gameObjects;
	};


	static_assert(sizeof(de::pointLight) == 48);
	static_assert(offsetof(de::GlobalUBO, pointLights) == 272);
	static_assert(offsetof(de::GlobalUBO, numLights) == 752);
	static_assert(sizeof(de::GlobalUBO) == 768);
}