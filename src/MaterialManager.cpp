#include<tiny_obj_loader.h>
#include<filesystem>
#include"DE_Device.hpp"
#include"DE_BindlessTextureRegistry.hpp"
#include"DE_MaterialBuffer.hpp"
#include"DE_MaterialData.hpp"
#include"Mesh/DE_MeshBaker.hpp"
#include"TextureManager.hpp"
#include"Material.hpp"
#include"MaterialManager.hpp"

namespace de {

	namespace {
			
			constexpr const char* kDefaultTexturePath = "Materials/Textures/Default.jpg";

			const std::string& pickNormalTexture(const tinyobj::material_t& material) {
				return material.normal_texname.empty() ? material.bump_texname : material.normal_texname;
			}


			std::string resolveTexturePath(const std::string& mtlBaseDir, const std::string& textureName) {
				return (std::filesystem::path(mtlBaseDir) / textureName).string();
			}

			std::string fixedToString(const std::array<char, 64>& fixed) {
				return std::string(fixed.data());
			}
			std::string fixedToString(const std::array<char, 128>& fixed) {
				return std::string(fixed.data());
			}
	}

	MaterialManager::MaterialManager(const Device& device, BindlessTextureRegistry& bindlessRegistry,
		MaterialBuffer& materialBuffer)
		:bindlessRegistry_{ bindlessRegistry }
		, materialBuffer_{ materialBuffer }
		, device_{ device }
	{
		textureManager_ = std::make_unique<TextureManager>(device_);

		std::shared_ptr<Texture> defaultTex = textureManager_->getTexture(kDefaultTexturePath);

		const uint32_t diffuseTexIndex = bindlessRegistry_.registerTexture(*defaultTex);

		GPUMaterialData defaultData{};
		defaultData.diffuseColor = glm::vec4(1.f, 1.f, 1.f, 1.f);
		defaultData.diffuseTexIndex = diffuseTexIndex;
		

		const uint32_t materialIndex = materialBuffer_.upload(defaultData);

		defaultMaterial_ = std::make_shared<Material>(defaultTex, materialIndex);

		cache_.try_emplace("__Default__", defaultMaterial_);
	}

	MaterialManager::~MaterialManager() = default;

	std::shared_ptr<Material> MaterialManager::getDefaultMaterial() {

		return defaultMaterial_;
	}

	std::shared_ptr<Material> MaterialManager::buildMaterial(
		const std::string& cacheKey,
		const std::string& diffuseTexName,
		const std::string& normalTexName,
		const glm::vec3& diffuseColor,
		const glm::vec3& specularColor,
		float dissolve,
		float shininess,
		const std::string& mtlBaseDir)
	{
		if (diffuseTexName.empty()) {
			return getDefaultMaterial();
		}

		auto [it, success] = cache_.try_emplace(cacheKey);

		std::shared_ptr<Material> sharedMaterial = it->second.lock();
		if (sharedMaterial) {
			return sharedMaterial;
		}

		std::shared_ptr<Texture> texture =
			textureManager_->getTexture(resolveTexturePath(mtlBaseDir, diffuseTexName));

		const uint32_t diffuseTexIndex = bindlessRegistry_.registerTexture(*texture);

		GPUMaterialData gpuData{};
		gpuData.diffuseColor = glm::vec4(diffuseColor, dissolve);
		gpuData.specularColor = glm::vec4(specularColor, shininess);
		gpuData.diffuseTexIndex = diffuseTexIndex;

		if (!normalTexName.empty()) {
			std::shared_ptr<Texture> normalTexture =
				textureManager_->getTexture(resolveTexturePath(mtlBaseDir, normalTexName));
			gpuData.normalTexIndex = bindlessRegistry_.registerTexture(*normalTexture);
		}
		// иначе normalTexIndex остаётся kInvalidTextureIndex (дефолт в GPUMaterialData)

		const uint32_t materialIndex = materialBuffer_.upload(gpuData);
		sharedMaterial = std::make_shared<Material>(texture, materialIndex);

		it->second = sharedMaterial;
		return sharedMaterial;
	}

	std::vector<std::shared_ptr<Material>> MaterialManager::resolveMaterials(const std::vector<MaterialRecord>& materials, const std::vector<int32_t>& defaultMaterialIds, const std::string& mtlBaseDir)
	{
		std::vector<std::shared_ptr<Material>> result;
		result.reserve(defaultMaterialIds.size());

		for (int32_t matId : defaultMaterialIds) {
			
			if (matId >= 0 && static_cast<size_t>(matId) < materials.size()) {
				result.push_back(getMaterial(materials[matId], mtlBaseDir));
			}
			else {
				result.push_back(getDefaultMaterial());
			}
		}
		return result;;
	}

	std::shared_ptr<Material> MaterialManager::getMaterial(const tinyobj::material_t& material, const std::string& mtlBaseDir) {

		return buildMaterial(
			mtlBaseDir + "|" + material.name,
			material.diffuse_texname,
			pickNormalTexture(material),
			glm::vec3(material.diffuse[0], material.diffuse[1], material.diffuse[2]),
			glm::vec3(material.specular[0], material.specular[1], material.specular[2]),
			static_cast<float>(material.dissolve),
			static_cast<float>(material.shininess),
			mtlBaseDir
		);
	}

	std::shared_ptr<Material> MaterialManager::getMaterial(const MaterialRecord& record, const std::string& mtlBaseDir) {

		const std::string name = fixedToString(record.name);
		const std::string diffuseTexName = fixedToString(record.diffuseTexturePath);
		const std::string normalTexName = fixedToString(record.normalTexturePath);

		return buildMaterial(
			mtlBaseDir + "|" + name,
			diffuseTexName,
			normalTexName,
			record.diffuseColor,
			record.specularColor,
			record.dissolve,
			record.shininess,
			mtlBaseDir
		);
	}
}
