#include<tiny_obj_loader.h>
#include"DE_Device.hpp"
#include"DE_Descriptors.hpp"
#include"TextureManager.hpp"
#include"Material.hpp"
#include"MaterialManager.hpp"

namespace de {
	MaterialManager::MaterialManager(const Device& device, const DescriptorPool& descriptorPool,
		const DescriptorSetLayout& descriptorLayout, std::string pathToMaterials)
		:pathToMaterials_{ pathToMaterials }
		,descriptorLayout_{descriptorLayout}
		,descriptorPool_{descriptorPool}
		,device_{device}
	{
	


		textureManager_ = std::make_unique<TextureManager>(device_);
		std::string defaultTexturePath = pathToMaterials_ + "/" + defaultTexture;

		std::shared_ptr<Texture> defaultTexture = textureManager_->getTexture(defaultTexturePath);

		auto descriptorInfo = defaultTexture->getDescriptorInfo();

		defaultMaterial_ = std::make_shared<Material>(defaultTexture,
			de::DescriptorWriter{ device_,descriptorPool_,descriptorLayout_ }
			.writeImage(0, &descriptorInfo)
			.build()
		);

		cache_.try_emplace("Default", defaultMaterial_);

	}

	MaterialManager::~MaterialManager() = default;
	
	std::shared_ptr<Material> MaterialManager::getDefaultMaterial() {
	
		return defaultMaterial_;
	}



	std::shared_ptr<Material> MaterialManager::getMaterial(tinyobj::material_t& material){

		std::string fullTexturePath = pathToMaterials_ + "/" + material.diffuse_texname;

		auto [it, success] = cache_.try_emplace(material.name);

		std::shared_ptr<Material> sharedMaterial = it->second.lock();

		if (!sharedMaterial) {
			std::shared_ptr<Texture> texture;
			if (!material.diffuse_texname.empty()) {
				texture = textureManager_->getTexture(pathToMaterials_ + "/" + material.diffuse_texname);
			}
			else {
				return getDefaultMaterial();
			}

			auto desciprorInfo = texture->getDescriptorInfo();
			sharedMaterial = std::make_shared<Material>(
				texture,
				de::DescriptorWriter{ device_,descriptorPool_,descriptorLayout_ }
				.writeImage(0, &desciprorInfo)
				.build()
			);

			it->second = sharedMaterial;

		}
		return sharedMaterial;
	}
}