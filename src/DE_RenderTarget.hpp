#ifndef _DE_RENDER_TARGET_
#define _DE_RENDER_TARGET_

#include<vulkan/vulkan.hpp>

namespace de {

	class RenderTarget {
	public:

		virtual ~RenderTarget() = default;
		
		virtual vk::Framebuffer getFramebuffer(uint32_t imageIndex) const = 0;
		virtual vk::RenderPass getRenderPass() const = 0;
		virtual vk::Extent2D getExtent() const = 0;
	};

}

#endif