#pragma once

//std
#include<memory>
#include<string>
#include<array>
#include<vector>

//vulkan
#include<vulkan/vulkan.hpp>

#include"DE_ITextureSource.hpp"

namespace de {

	class Device;
	class Image;

	class Texture final : public ITextureSource {

	public:

		Texture(std::string pathToTexture,
			const Device& device,
			vk::Format textureFormat = vk::Format::eR8G8B8A8Srgb,
			vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eClampToEdge,
			bool enableAnisotropy = true,
			vk::ImageType imgType = vk::ImageType::e2D);
		Texture(const Device& device, vk::Format textureFormat,
			void* data, uint32_t width, uint32_t height,
			vk::SamplerAddressMode addressMode = vk::SamplerAddressMode::eClampToEdge,
			bool enableAnisotropy = true,
			vk::ImageType imgType = vk::ImageType::e2D);

		static Texture loadCubemap(const Device& device, std::string pathToTexture);

		~Texture();
		Texture(const Texture&) = delete;
		Texture& operator=(const Texture&) = delete;

		Texture(Texture&&) = default;
		Texture& operator=(Texture&&) = default;

		const de::Image& getImage() const noexcept {
			return *textureImage_;
		}
		const vk::ImageView& getImageView() const noexcept {
			return*textureImageView_;
		}
		const vk::Sampler& getSampler() const noexcept {
			return *textureSampler_;
		}

		vk::DescriptorImageInfo getDescriptorInfo() const noexcept override {

			return vk::DescriptorImageInfo{}
				.setImageLayout(vk::ImageLayout::eShaderReadOnlyOptimal)
				.setImageView(*textureImageView_)
				.setSampler(*textureSampler_);
		}


	private:

		Texture(const Device& device,
			vk::Format imgFormat,
			uint32_t faceWidht,
			uint32_t faceHeight,
			const std::array<std::vector<uint8_t>, 6>& facePixels);

		std::unique_ptr<Image> textureImage_;
		vk::UniqueImageView textureImageView_;
		vk::UniqueSampler textureSampler_;
	};
}
