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
	class BindlessTextureRegistry;
	class MaterialBuffer;
	struct MaterialRecord;

	class MaterialManager {
	public:

		MaterialManager(const Device& device,
			BindlessTextureRegistry& bindlessRegistry,
			MaterialBuffer& materialBuffer);

		~MaterialManager();
		MaterialManager(const MaterialManager&) = delete;
		MaterialManager& operator=(const MaterialManager&) = delete;

		MaterialManager(MaterialManager&&) = default;
		MaterialManager& operator=(MaterialManager&&) = default;

		std::vector<std::shared_ptr<Material>> resolveMaterials(
			const std::vector<MaterialRecord>& materials,
			const std::vector<int32_t>& defaultMaterialIds,
			const std::string& mtlBaseDir);
		std::shared_ptr<Material> getMaterial(const tinyobj::material_t& material, const std::string& mtlBaseDir);
		std::shared_ptr<Material> getMaterial(const MaterialRecord& record, const std::string& mtlBaseDir);
		std::shared_ptr<Material> getDefaultMaterial();

	private:

		std::shared_ptr<Material> buildMaterial(
			const std::string& cacheKey,
			const std::string& diffuseTexName,
			const std::string& normalTexName,
			const glm::vec3& diffuseColor,
			const glm::vec3& specularColor,
			float dissolve,
			float shininess,
			const std::string& mtlBaseDir);

		std::shared_ptr<Material> defaultMaterial_;

		std::unordered_map <std::string, std::weak_ptr<Material>> cache_;

		std::unique_ptr<TextureManager> textureManager_;

		BindlessTextureRegistry& bindlessRegistry_;
		MaterialBuffer& materialBuffer_;
		const Device& device_;

	};
}
