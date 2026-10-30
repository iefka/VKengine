#pragma once

#include<cstdint>
#include<vulkan/vulkan.hpp>

namespace de {
	class IComputeStep {
	public:
		virtual ~IComputeStep() = default;
		virtual void dispatch(vk::CommandBuffer commandByuffer, uint32_t frameIndex) = 0;
	};

}