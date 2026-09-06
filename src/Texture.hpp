#pragma once

//std
#include<memory>

//vulkan
#include<vulkan/vulkan.hpp>

namespace de {

	class Device;
	class Image;

	class Texture {

	public:

		Texture(std::string pathToTexture, const Device& device);

		~Texture();
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		Texture(Texture&&) = default;
		Texture& operator=(Texture&&) = default;

		const de::Image& getImage() const noexcept {
			return *textureImage_;
		}
		const vk::ImageView& getImageView() const noexcept {
			return *textureImageView_;
		}
		const vk::Sampler& getSampler() const noexcept {
			return *textureSampler_;
		}

		vk::DescriptorImageInfo getDescriptorInfo() const noexcept{

			return vk::DescriptorImageInfo{}
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(*textureImageView_)
				.setSampler(*textureSampler_);
		}
			

	private:
		std::unique_ptr<Image> textureImage_;
		vk::UniqueImageView textureImageView_;
		vk::UniqueSampler textureSampler_;
	};
}