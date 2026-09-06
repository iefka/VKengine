#pragma once

#include<string>
#include<unordered_map>
#include<memory>

namespace tinyobj {
	struct material_t;
}
namespace de {

	class Texture;
	class TextureManager;
	class Device;
	class Material;
	class DescriptorPool;
	class DescriptorSetLayout;
	

	class MaterialManager {
	public:

		MaterialManager(const Device& device, const DescriptorPool& descriptorPool,
			const DescriptorSetLayout& descriptorLayout, std::string pathToMaterials);


		~MaterialManager();
		MaterialManager(const MaterialManager&) = delete;
		MaterialManager& operator=(const MaterialManager&) = delete;

		MaterialManager(MaterialManager&&) = default;
		MaterialManager& operator=(MaterialManager&&) = default;

		std::shared_ptr<Material> getMaterial(tinyobj::material_t& material);
		std::shared_ptr<Material> getDefaultMaterial();

		std::string getMaterialsPath() const noexcept {
			return pathToMaterials_;
		}
		
	private:

		std::shared_ptr<Material> defaultMaterial_;
		std::unordered_map <std::string, std::weak_ptr<Material>> cache_;

		const std::string pathToMaterials_;

		//default material owner
		const std::string defaultTexture{ "Textures/Default.jpg" };

		std::unique_ptr<TextureManager> textureManager_;

		const DescriptorPool& descriptorPool_;
		const DescriptorSetLayout& descriptorLayout_;
		const Device& device_;

	};
}
