#include"DE_Device.hpp"
#include"DE_Memory.hpp"
#include "Texture.hpp"
#include"Utility/DE_Utility.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "third_party/stb_image.h"
#include<ranges>

namespace de {
	

	Texture::~Texture() = default;
	Texture::Texture(std::string pathToTexture,
		const Device& device,
		vk::ImageType imgType) {

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
			.setImageType(imgType)
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


		auto region = vk::BufferImageCopy{}
			.setImageSubresource({ vk::ImageAspectFlagBits::eColor,0,0,1 })
			.setImageExtent(vk::Extent3D{
				static_cast<uint32_t>(texWidth),
				static_cast<uint32_t>(texHeight),
				1 });

		device.copyBufferToImage(*stagingBuffer.getBuffer(),
			textureImage_->getImage(), 
			region
		);

		device.transitionImageLayout(textureImage_->getImage(), textureFormat,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		textureImageView_ = utl::createImageView(device.getLogicalDevice(), textureImage_->getImage(),
			textureFormat, vk::ImageAspectFlagBits::eColor);

		textureSampler_ = utl::createTextureSampler(device.getLogicalDevice());

	}

	Texture::Texture(const Device& device, vk::Format textureFormat,
		void* data, uint32_t width, uint32_t height,
		vk::ImageType imgType){
	

		uint32_t pixelSize = utl::getFormatSize(textureFormat);

		vk::DeviceSize imageSize = width * height * pixelSize;
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

		ImageUsageInfo imageUsage{
			imageSize,
			0,
			vk::ImageCreateInfo{}
			.setImageType(imgType)
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

		auto region = vk::BufferImageCopy{}
			.setImageSubresource({ vk::ImageAspectFlagBits::eColor,0,0,1 })
			.setImageExtent({ width,height,1 });

		device.copyBufferToImage(*stagingBuffer.getBuffer(),
			textureImage_->getImage(),
			region
		);

		device.transitionImageLayout(textureImage_->getImage(), textureFormat,
			vk::ImageLayout::eTransferDstOptimal, vk::ImageLayout::eShaderReadOnlyOptimal);

		textureImageView_ = utl::createImageView(device.getLogicalDevice(), textureImage_->getImage(),
			textureFormat, vk::ImageAspectFlagBits::eColor);

		textureSampler_ = utl::createTextureSampler(device.getLogicalDevice());
	}


	//extract tile requested size
	std::vector<uint8_t> extractTile(const uint8_t* srcPixels, uint32_t srcWidth, uint32_t srcHeight,
		uint32_t srcX, uint32_t srcY, uint32_t tileWidth, uint32_t tileHeight) {

		if (srcX + tileWidth > srcWidth || srcY + tileHeight > srcHeight) {
			throw std::runtime_error("extractTile: requested tile is out of source image bounds");
		}

		constexpr uint32_t bytesPerPixel = 4;
		const uint32_t rowBytes = tileWidth * bytesPerPixel;

		std::vector<uint8_t> tile(static_cast<size_t>(tileHeight) * rowBytes);

		for (uint32_t row = 0; row < tileHeight; ++row) {
			const uint8_t* srcRow = srcPixels + ((srcY + row) * srcWidth + srcX) * bytesPerPixel;
			uint8_t* dstRow = tile.data() + row * rowBytes;
			std::memcpy(dstRow, srcRow, rowBytes);
		}

		return tile;
	}

	Texture Texture::loadCubemap(
		const Device& device,
		std::string pathToTexture){
		
		constexpr vk::Format format{ vk::Format::eR8G8B8A8Srgb };
		int texWidth{}, texHeight{}, texChannels{};
		stbi_uc* pixels = stbi_load(pathToTexture.c_str(), &texWidth,
			&texHeight, &texChannels, STBI_rgb_alpha);

		if (!pixels) {
			throw std::runtime_error("cubemap image data is nullptr: " + pathToTexture);
		}

		int tileW = texWidth / 4;
		int tileH = texHeight / 3;
		if (tileW != tileH) {
			stbi_image_free(pixels);
			throw std::runtime_error("cubemap tiles are not square: " + pathToTexture);
		}

		struct FaceCoord { int col; int row; };
		static constexpr std::array<FaceCoord, 6> faceCoords = {{
			{2,1},
			{0,1},
			{1,0},
			{1,2},
			{1,1},
			{3,1}
		}};

		std::array<std::vector<uint8_t>, 6> facePixels;

		for (auto&& [index,face]: std::views::enumerate(facePixels)) {
			face = extractTile(
				pixels,
				texWidth,
				texHeight,
				faceCoords[index].col * tileW,
				faceCoords[index].row * tileH,
				tileW,
				tileH
			);
		}
		stbi_image_free(pixels);
		return Texture(device, format, 
			static_cast<uint32_t>(tileW), 
			static_cast<uint32_t>(tileH),
			facePixels);
	}

	Texture::Texture(
		const Device& device,
		vk::Format imgFormat,
		uint32_t faceWidht,
		uint32_t faceHeight,
		const std::array<std::vector<uint8_t>, 6>& facePixels) {


		constexpr uint32_t faceCount{ 6 };
		uint32_t pixelSize = utl::getFormatSize(imgFormat);

		vk::DeviceSize faceSize = faceWidht * faceHeight * pixelSize;
		vk::DeviceSize imageSize = faceSize * faceCount;


		MemoryUsageInfo stagingBuferInfo{
			imageSize,
			1,
			0,
			vk::BufferUsageFlagBits::eTransferSrc
		};

		Buffer stagingBuffer(device, stagingBuferInfo);
		stagingBuffer.map();

		for (auto&& [index,face] : std::views::enumerate(facePixels))
		{
			stagingBuffer.copyToBuffer(face.data(), faceSize, faceSize * index);

		}

		ImageUsageInfo imageUsage{
			imageSize,
			0,
			vk::ImageCreateInfo{}
			.setImageType(vk::ImageType::e2D)
			.setFlags(vk::ImageCreateFlagBits::eCubeCompatible)
			.setFormat(imgFormat)
			.setExtent(vk::Extent3D{
				faceWidht,
				faceHeight,
				1})
			.setMipLevels(1)
			.setArrayLayers(faceCount)
			.setTiling(vk::ImageTiling::eOptimal)
			.setUsage(vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled)
			.setInitialLayout(vk::ImageLayout::eUndefined)
			.setSamples(vk::SampleCountFlagBits::e1)
			.setSharingMode(vk::SharingMode::eExclusive),
			vk::MemoryPropertyFlagBits::eDeviceLocal
		};

		textureImage_ = std::make_unique<Image>(device, imageUsage);

		device.transitionImageLayout(textureImage_->getImage(), imgFormat,
			vk::ImageLayout::eUndefined,
			vk::ImageLayout::eTransferDstOptimal, faceCount);


		auto region = vk::BufferImageCopy{}
			.setImageSubresource({ vk::ImageAspectFlagBits::eColor,0,0,faceCount })
			.setImageExtent({ faceWidht,faceHeight,1 });
		device.copyBufferToImage(*stagingBuffer.getBuffer(), textureImage_->getImage(), region);

		device.transitionImageLayout(textureImage_->getImage(), imgFormat,
			vk::ImageLayout::eTransferDstOptimal,
			vk::ImageLayout::eShaderReadOnlyOptimal, faceCount);

		textureImageView_ = utl::createImageView(device.getLogicalDevice(), textureImage_->getImage(),
			imgFormat, vk::ImageAspectFlagBits::eColor,vk::ImageViewType::eCube,faceCount);

		textureSampler_ = utl::createTextureSampler(device.getLogicalDevice(),
			vk::SamplerAddressMode::eClampToEdge,
			false);

	}
}
	