#include<vulkan/vulkan.hpp>
#include"DE_GameObject.hpp"
#include "DE_Camera.hpp"
namespace de {
	

	struct FrameInfo
	{
		std::uint32_t frameIndex;
		vk::CommandBuffer commandBuffer;
		vk::DescriptorSet descriptorSet;
		const de::Camera& camera;
		const GameObject::Map& gameObjects;
	};
}