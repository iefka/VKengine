#include"DE_Device.hpp"
#include"DE_Memory.hpp"
#include "Texture.hpp"
#include"Utility/DE_Utility.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"


namespace de {
	vk::UniqueSampler createTextureSampler(const vk::Device& logicalDevice) {

		const auto samplerInfo = vk::SamplerCreateInfo{}
			.setAddressModeU(vk::SamplerAddressMode::eRepeat)
			.setAddressModeV(vk::SamplerAddressMode::eRepeat)
			.setAddressModeW(vk::SamplerAddressMode::eRepeat)
			.setMagFilter(vk::Filter::eLinear)
			.setMinFilter(vk::Filter::eLinear)
			.setAnisotropyEnable(vk::True)
			.setMaxAnisotropy(4.0)
			.setBorderColor(vk::BorderColor::eIntOpaqueBlack)
			.setUnnormalizedCoordinates(vk::False)
			.setCompareEnable(vk::False)
			.setCompareOp(vk::CompareOp::eAlways)
			.setMipmapMode(vk::SamplerMipmapMode::eLinear)
			.setMipLodBias(0.f)
			.setMinLod(0.f)
			.setMaxLod(0.f);

		return logicalDevice.createSamplerUnique(samplerInfo);
	}

	Texture::~Texture() = default;
	Texture::Texture(std::string pathToTexture, const Device& device) {

		int texWidth{}, texHeight{}, texChannels{};

		vk::Format textureFormat = vk::Format::eR8G8B8A8Srgb;
		stbi_uc* pixels = stbi_load(pathToTexture.c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

		// RGBA -  1byte for each color so 4 bytes per pixel
		vk::DeviceSize imageSize = texWidth * texHeight * 4;
		if (!pixels) {
			throw std::runtime_error("texture data is nullptr");
		}

		MemoryUsageInfo stagingBuferInfo{
			imageSize,
			1,
			0,
			vk::BufferUsageFlagBits::eTransferSrc
		};

		Buffer stagingBuffer(device, stagingBuferInfo);

		stagingBuffer.map();
		stagingBuffer.copyToBuffer(pixels, imageSize, 0);
		stbi_image_free(pixels);

		ImageUsageInfo imageUsage{
			imageSize,
			0,
			vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setFormat(textureFormat)
			.setExtent(vk::Extent3D{
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight),
				1})
			.setMipLevels(1)
			.setArrayLayers(1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setSharingMode(vk::SharingMode::eExclusive),
			vk::MemoryPropertyFlagBits::eDeviceLocal
		};

		textureImage_ = std::make_unique<Image>(device, imageUsage);

		device.transitionImageLayout(textureImage_->getImage(), textureFormat,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		device.copyBufferToImage(*stagingBuffer.getBuffer(),
			textureImage_->getImage(), 
			static_cast<uint32_t>(texWidth), 
			static_cast<uint32_t>(texHeight)
		);

		device.transitionImageLayout(textureImage_->getImage(), textureFormat,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		textureImageView_ = utl::createImageView(device.getLogicalDevice(), textureImage_->getImage(),
			textureFormat, vk::ImageAspectFlagBits::eColor);

		textureSampler_ = createTextureSampler(device.getLogicalDevice());

	}

	Texture::Texture(const Device& device, vk::Format textureFormat,
		void* data, uint32_t width, uint32_t height){
	

		uint32_t formatSize = utl::getFormatSize(textureFormat);

		vk::DeviceSize imageSize = width * height * formatSize;
		if (!data) {
			throw std::runtime_error("texture data is nullptr");
		}

		MemoryUsageInfo stagingBuferInfo{
			imageSize,
			1,
			0,
			vk::BufferUsageFlagBits::eTransferSrc
		};

		Buffer stagingBuffer(device, stagingBuferInfo);

		stagingBuffer.map();
		stagingBuffer.copyToBuffer(data, imageSize, 0);
		stbi_image_free(data);

		ImageUsageInfo imageUsage{
			imageSize,
			0,
			vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setFormat(textureFormat)
			.setExtent(vk::Extent3D{
				width,
				height,
				1})
			.setMipLevels(1)
			.setArrayLayers(1)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setSharingMode(vk::SharingMode::eExclusive),
			vk::MemoryPropertyFlagBits::eDeviceLocal
		};

		textureImage_ = std::make_unique<Image>(device, imageUsage);

		device.transitionImageLayout(textureImage_->getImage(), textureFormat,
			vk::ImageLayout::eUndefined, vk::ImageLayout::eTransferDstOptimal);

		device.copyBufferToImage(*stagingBuffer.getBuffer(),
			textureImage_->getImage(),
			width,
			height
		);

		device.transitionImageLayout(textureImage_->getImage(), textureFormat,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		textureImageView_ = utl::createImageView(device.getLogicalDevice(), textureImage_->getImage(),
			textureFormat, vk::ImageAspectFlagBits::eColor);

		textureSampler_ = createTextureSampler(device.getLogicalDevice());
	}
	
}