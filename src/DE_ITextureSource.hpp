#pragma once

#include<vulkan/vulkan.hpp>

namespace de {

	class ITextureSource {
	public:
		virtual ~ITextureSource() = default;
		virtual vk::DescriptorImageInfo getDescriptorInfo() const noexcept = 0;
	};

	class ComputeTextureView final : public ITextureSource  {
		public:
			ComputeTextureView(
				vk::Sampler sampler,
				vk::ImageView imgView,
				vk::ImageLayout imgLayout)
				:sampler_{sampler}
				,imgView_{imgView}
				,imgLayout_{imgLayout} {}

			vk::DescriptorImageInfo getDescriptorInfo() const noexcept override{
				return vk::DescriptorImageInfo{ sampler_,imgView_,imgLayout_ };
			}
	private:

		vk::Sampler sampler_;
		vk::ImageView imgView_;
		vk::ImageLayout imgLayout_;

	};
}