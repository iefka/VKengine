#pragma once


#include<array>
#include<vector>
#include<filesystem>
#include<expected>

#include "glm_config.hpp"

namespace de {
	inline constexpr uint32_t kMaxLODs = 8;
	inline constexpr uint32_t kMeshFileMagic = 0x4853454Du;
	inline constexpr uint32_t kMeshFileVersion = 2;

	struct BoundingBox{

		//not usable init values
		glm::vec3 min{ std::numeric_limits<float>::max() };
		glm::vec3 max{ -std::numeric_limits<float>::max() };

		glm::vec3 center() const noexcept { return (min + max) * 0.5f; }
		glm::vec3 halfExtents() const noexcept { return (max - min) * 0.5f; }

		void extend(const glm::vec3& p) noexcept {
			min = glm::min(min, p);
			max = glm::max(max, p);
		}
		void extend(const BoundingBox& other) noexcept {
			extend(other.max);
			extend(other.min);
		}
	};

	struct MeshDesc {
		uint32_t lodCount = 1;
		uint32_t indexOffset = 0;
		uint32_t vertexOffset = 0;
		uint32_t vertexCount = 0;

		std::array<uint32_t, kMaxLODs + 1> lodOffset{};

		uint32_t getLodIndexCount(uint32_t lod) const noexcept {
			return lodOffset[lod + 1] - lodOffset[lod];
		}
	};
	struct MeshData {
			std::vector<uint32_t> indices;
			std::vector<glm::vec3> positions;
			std::vector<glm::vec3> normals;
			std::vector<glm::vec3> colors;
			std::vector<glm::vec2> UVs;

			std::vector<MeshDesc> meshes;
			std::vector<BoundingBox> boxes;
	};

	enum class MeshIOError {
			FileNotFound,
			BadMagic,
			UnsuportedVersion,
			TruncatedFile,
			WriteFailed
	};

	std::expected<MeshData, MeshIOError> loadMeshData(const std::filesystem::path& path);
	std::expected<void, MeshIOError> saveMeshData(const std::filesystem::path& path, const MeshData& data);
	void recalculateBoundingBoxes(MeshData& data);
	consteval MeshData mergeMeshData(const std::vector<const MeshData*>& parts);
	};