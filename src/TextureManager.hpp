#pragma once


#include<unordered_map>
#include<string>
#include<memory>


namespace de {

	class Texture;
	class Device;

	class TextureManager {
	public:
		TextureManager(const Device& device):device_{device}{}

		std::shared_ptr<Texture> getTexture(std::string texturePath);

	private:
		std::unordered_map<std::string, std::weak_ptr<Texture>> cache_;

		const Device& device_;
	};
}