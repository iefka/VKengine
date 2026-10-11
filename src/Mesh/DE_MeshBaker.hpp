#pragma once

#include"DE_MeshData.hpp"
#include<string>
#include<array>
#include<filesystem>

namespace de {
	struct MaterialRecord {
		std::array<char, 64> name{};
		std::array<char, 128> diffuseTexturePath{};
		glm::vec3 diffuseColor{ 0.6f, 0.6f, 0.6f };
	};

	struct BakedMesh {
		MeshData geometry;
		std::vector<MaterialRecord> materials;
		std::vector<int32_t> defaultMaterialsIds;
		std::string mtlBaseDir;
	};

	std::expected<BakedMesh, std::string> bakeMeshFromObj(const std::filesystem::path& objFilepath);
	std::expected<BakedMesh, std::string> loadOrBakeMesh(const std::filesystem::path& objFilepath);

}