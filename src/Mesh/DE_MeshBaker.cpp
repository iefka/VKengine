#define TINYOBJLOADER_USE_MULTITHREADING
#define TINYOBJLOADER_USE_SIMD
#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_EXTENSION
#include<tiny_obj_loader.h>

#include "DE_MeshBaker.hpp"
#include"DE_BinaryIO.hpp"
#include"Utility/DE_Utility.hpp"

#include<filesystem>
#include<unordered_map>
#include<map>
#include<fstream>
#include<cstring>
#include<ranges>
#include<algorithm>
#include<cstddef>
#include<cstdint>
#include<expected>
#include<iostream>
#include<stdexcept>
#include<cmath>

namespace {
    struct VertexKey {
        glm::vec3 position{ 0.0f };
        glm::vec3 normal{ 0.0f };
        glm::vec2 uv{ 0.0f };
        bool operator==(const VertexKey&) const = default;
    };

    struct GeneratedNormals {
        std::vector<glm::vec3> values;
        std::size_t generatedPositions = 0;
        std::size_t skippedDegenerateTriangles = 0;
    };

    // Compute area-weighted smooth normals keyed by OBJ position index, not
    // by the final deduplicated vertex. That shares normals across UV seams.
    std::expected<GeneratedNormals, std::string> generateSmoothNormals(
        const tinyobj::basic_attrib_t<>& attrib,
        const std::vector<tinyobj::basic_shape_t<>>& shapes) {

        if (attrib.vertices.size() % 3 != 0) {
            return std::unexpected("[normal-fallback] malformed position array");
        }

        const std::size_t positionCount = attrib.vertices.size() / 3;
        std::vector<glm::dvec3> sums(positionCount, glm::dvec3{ 0.0 });
        std::vector<std::uint8_t> referenced(positionCount, 0);
        std::size_t skippedDegenerate = 0;

        for (const auto& shape : shapes) {
            std::size_t faceOffset = 0;
            for (std::size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
                const std::size_t faceVertexCount = shape.mesh.num_face_vertices[face];
                if (faceVertexCount != 3) {
                    return std::unexpected("[normal-fallback] expected triangulated faces: " + shape.name);
                }
                if (faceOffset > shape.mesh.indices.size() ||
                    faceVertexCount > shape.mesh.indices.size() - faceOffset) {
                    return std::unexpected("[normal-fallback] face indices out of range: " + shape.name);
                }

                std::size_t posIndices[3]{};
                glm::dvec3 p[3]{};
                for (std::size_t corner = 0; corner < 3; ++corner) {
                    const auto rawIndex = shape.mesh.indices[faceOffset + corner].vertex_index;
                    if (rawIndex < 0 || static_cast<std::size_t>(rawIndex) >= positionCount) {
                        return std::unexpected("[normal-fallback] invalid position index: " + shape.name);
                    }
                    const auto positionIndex = static_cast<std::size_t>(rawIndex);
                    posIndices[corner] = positionIndex;
                    referenced[positionIndex] = 1;
                    const auto base = positionIndex * 3;
                    p[corner] = glm::dvec3{
                        static_cast<double>(attrib.vertices[base]),
                        static_cast<double>(attrib.vertices[base + 1]),
                        static_cast<double>(attrib.vertices[base + 2])
                    };
                    if (!std::isfinite(p[corner].x) || !std::isfinite(p[corner].y) ||
                        !std::isfinite(p[corner].z)) {
                        return std::unexpected("[normal-fallback] non-finite position: " + shape.name);
                    }
                }

                // The unnormalized cross product contributes by triangle area.
                const glm::dvec3 faceNormal = glm::cross(p[1] - p[0], p[2] - p[0]);
                const double faceLen2 = glm::dot(faceNormal, faceNormal);
                if (!std::isfinite(faceLen2)) {
                    return std::unexpected("[normal-fallback] non-finite face normal: " + shape.name);
                }
                if (faceLen2 == 0.0) {
                    ++skippedDegenerate;
                }
                else {
                    for (const auto positionIndex : posIndices) {
                        sums[positionIndex] += faceNormal;
                    }
                }
                faceOffset += faceVertexCount;
            }
            if (faceOffset != shape.mesh.indices.size()) {
                return std::unexpected("[normal-fallback] face/index count mismatch: " + shape.name);
            }
        }

        GeneratedNormals result{};
        result.values.resize(positionCount, glm::vec3{ 0.0f });
        result.skippedDegenerateTriangles = skippedDegenerate;
        for (std::size_t i = 0; i < positionCount; ++i) {
            if (!referenced[i]) continue;
            const double len2 = glm::dot(sums[i], sums[i]);
            if (!(len2 > 0.0) || !std::isfinite(len2)) {
                return std::unexpected(
                    "[normal-fallback] cannot compute a smooth normal for OBJ position " +
                    std::to_string(i) + " (degenerate or cancelling adjacent faces)");
            }
            result.values[i] = glm::vec3(sums[i] / std::sqrt(len2));
            ++result.generatedPositions;
        }
        return result;
    }
}

namespace std {
    template<>
    struct hash<VertexKey> {
        size_t operator()(const VertexKey& k) const {
            size_t seed{};
            de::utl::hashCombine(seed, k.position, k.normal, k.uv);
            return seed;
        }
    };
}

namespace de {
    constexpr uint32_t kMatCacheMagic = 0x4154414Du;
    constexpr uint32_t kMatCacheVersion = 3;

    struct MaterialCacheHeader {
        uint32_t magic;
        uint32_t version;
    };

    struct MaterialCacheData {
        std::vector<MaterialRecord> materials;
        std::vector<int32_t> defaultMaterialIDs;
    };

    void copyToFixed(std::array<char, 64>& dst, const std::string& src) {
        std::memset(dst.data(), 0, dst.size());
        std::memcpy(dst.data(), src.data(), std::min(src.size(), dst.size() - 1));
    }
    void copyToFixed(std::array<char, 128>& dst, const std::string& src) {
        std::memset(dst.data(), 0, dst.size());
        std::memcpy(dst.data(), src.data(), std::min(src.size(), dst.size() - 1));
    }

    std::expected<MaterialCacheData, std::string> loadMaterialCache(const std::filesystem::path& path) {
        std::ifstream in(path, std::ios::binary);

        if (!in) {
            return std::unexpected("material cache not found" + path.string());
        }

        MaterialCacheHeader header{};
        in.read(reinterpret_cast<char*>(&header), sizeof(header));

        if (!in || header.magic != kMatCacheMagic) {
            return std::unexpected("bad material cache" + path.string());
        }
        if (header.version != kMatCacheVersion) {
            return std::unexpected("outdated material cache version" + path.string());
        }
        try {
            MaterialCacheData data;
            data.materials = io::readVector<MaterialRecord>(in);
            data.defaultMaterialIDs = io::readVector<int32_t>(in);
            return data;
        }
        catch (const std::runtime_error& error) {
            return std::unexpected(std::string("failed to write ") + error.what());
        }

    }

    std::expected<void, std::string> saveMaterialCache(const std::filesystem::path& path,
        const MaterialCacheData& data) {

        std::ofstream out(path, std::ios::binary);
        if (!out) {
            return std::unexpected("failed to open " + path.string() + "for writing");
        }

        const MaterialCacheHeader header{ kMatCacheMagic,kMatCacheVersion };
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));
        io::writeVector(out, data.materials);
        io::writeVector(out, data.defaultMaterialIDs);

        if (!out) {
            return std::unexpected("failed to write" + path.string());
        }
        return{};
    }

    bool cacheIsFresh(
        const std::filesystem::path& objPath,
        const std::filesystem::path& matCachePath,
        const std::filesystem::path& meshCachePath) {

        std::error_code ec;

        if (!std::filesystem::exists(objPath)
            || !std::filesystem::exists(matCachePath)
            || !std::filesystem::exists(meshCachePath)) {
            return false;
        }
        const auto objTime = std::filesystem::last_write_time(objPath, ec);
        const auto matTime = std::filesystem::last_write_time(matCachePath, ec);
        const auto meshTime = std::filesystem::last_write_time(meshCachePath, ec);

        if (ec) {
            return false;
        }

        return
            matTime > objTime && meshTime > objTime;
    }

    std::expected<BakedMesh, std::string> bakeMeshFromObj(const std::filesystem::path& objFilepath) {

        tinyobj::basic_attrib_t<> attrib;
        std::vector<tinyobj::basic_shape_t<>> shapes;
        std::vector<tinyobj::material_t> rawMaterials;
        std::string warn, error;

        const std::string mtlBaseDir = objFilepath.parent_path().string();

        tinyobj::OptLoadConfig optLoadCongig{};
        // -1 = hardware_concurrency
        optLoadCongig.num_threads = -1;
        optLoadCongig.triangulate = true;

        if (!tinyobj::LoadObjOpt(&attrib, &shapes, &rawMaterials, &warn, &error,
            objFilepath.string().c_str(), mtlBaseDir.c_str(), optLoadCongig)) {
            return std::unexpected(error);
        }

  

        bool hasReferencedNormals = false;
        bool hasMissingNormals = false;
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                if (index.normal_index >= 0) hasReferencedNormals = true;
                else hasMissingNormals = true;
            }
        }

        std::vector<glm::vec3> generatedNormals;
        bool useGeneratedNormals = false;
        if (hasMissingNormals && !hasReferencedNormals) {
            auto generated = generateSmoothNormals(attrib, shapes);
            if (!generated) return std::unexpected(generated.error());
            std::cout << "[normal-fallback] " << objFilepath.filename().string()
                << " generatedPositions=" << generated->generatedPositions
                << " skippedDegenerateTriangles=" << generated->skippedDegenerateTriangles
                << '\n';
            generatedNormals = std::move(generated->values);
            useGeneratedNormals = true;
        }
        else if (hasMissingNormals && hasReferencedNormals) {
            std::cerr << "[normal-fallback] " << objFilepath.filename().string()
                << ": mixed provided/missing normals; existing normals preserved, "
                "missing corners are not generated in this version\n";
        }

        MeshData geometry;

        std::unordered_map<VertexKey, uint32_t> uniqueVertices;
        std::map<int, std::vector<uint32_t>> indicesByMaterial;

        // Temporary diagnostics: these count OBJ corners, not unique vertices.
        std::size_t checkedCorners = 0;
        std::size_t missingNormals = 0;
        std::size_t missingUVs = 0;


        for (const auto& shape : shapes) {
            if (!shape.mesh.material_ids.empty() &&
                shape.mesh.material_ids.size() < shape.mesh.num_face_vertices.size()) {
                throw std::runtime_error("[mesh-check] material_ids shorter than faces: " + shape.name);
            }
            size_t faceOffset = 0;

            for (size_t face = 0; face < shape.mesh.num_face_vertices.size(); ++face) {
                const uint32_t faceVertexCount = shape.mesh.num_face_vertices[face];
                const int materialID = shape.mesh.material_ids.empty() ? -1 :
                    shape.mesh.material_ids[face];

                auto& bucket = indicesByMaterial[materialID];

                for (uint32_t vertex = 0; vertex < faceVertexCount; ++vertex) {
                    if (faceOffset > shape.mesh.indices.size() ||
                        vertex >= shape.mesh.indices.size() - faceOffset) {
                        throw std::runtime_error("[mesh-check] face index out of range: " + shape.name);
                    }
                    const auto& idx = shape.mesh.indices[faceOffset + vertex];

                    if (idx.vertex_index < 0 ||
                        static_cast<std::size_t>(idx.vertex_index) >= attrib.vertices.size() / 3) {
                        throw std::runtime_error("[mesh-check] invalid position index: " + shape.name);
                    }
                    if (idx.normal_index >= 0 &&
                        static_cast<std::size_t>(idx.normal_index) >= attrib.normals.size() / 3) {
                        throw std::runtime_error("[mesh-check] normal index out of range: " + shape.name);
                    }
                    if (idx.texcoord_index >= 0 &&
                        static_cast<std::size_t>(idx.texcoord_index) >= attrib.texcoords.size() / 2) {
                        throw std::runtime_error("[mesh-check] UV index out of range: " + shape.name);
                    }
                    if (idx.normal_index < 0) ++missingNormals;
                    if (idx.texcoord_index < 0) ++missingUVs;

                    VertexKey key;
                    if (idx.vertex_index >= 0) {
                        key.position = {
                            attrib.vertices[3 * idx.vertex_index + 0],
                            attrib.vertices[3 * idx.vertex_index + 1],
                            attrib.vertices[3 * idx.vertex_index + 2]
                        };
                    }
                    if (idx.normal_index >= 0) {
                        key.normal = {
                            attrib.normals[3 * idx.normal_index + 0],
                            attrib.normals[3 * idx.normal_index + 1],
                            attrib.normals[3 * idx.normal_index + 2]
                        };
                    }
                    else if (useGeneratedNormals) {
                        key.normal = generatedNormals[static_cast<std::size_t>(idx.vertex_index)];
                    }
                    if (idx.texcoord_index >= 0) {
                        key.uv = {
                            attrib.texcoords[2 * idx.texcoord_index + 0],
                            attrib.texcoords[2 * idx.texcoord_index + 1]
                        };
                    }

                    auto [it, inserted] = uniqueVertices.try_emplace(key,
                        static_cast<uint32_t>(geometry.positions.size()));

                    if (inserted) {
                        geometry.positions.push_back(key.position);
                        geometry.normals.push_back(key.normal);
                        geometry.colors.push_back({ 1.f,1.f,1.f });
                        geometry.UVs.push_back(key.uv);
                    }

                    const auto outIndex = static_cast<std::size_t>(it->second);
                    if (outIndex >= geometry.positions.size() ||
                        outIndex >= geometry.normals.size() ||
                        outIndex >= geometry.UVs.size() ||
                        geometry.positions[outIndex] != key.position ||
                        geometry.normals[outIndex] != key.normal ||
                        geometry.UVs[outIndex] != key.uv) {
                        throw std::runtime_error(
                            "[mesh-check] dedup mismatch: shape=" + shape.name +
                            " face=" + std::to_string(face) +
                            " corner=" + std::to_string(vertex));
                    }
                    ++checkedCorners;
                    bucket.push_back(it->second);
                } //vertices
                faceOffset += faceVertexCount;
            }//faces
        }//shapes

        std::vector<int32_t> defaultMaterialIDs;

        for (const auto& [materialID, bucket] : indicesByMaterial) {
            MeshDesc desc{};
            desc.lodCount = 1;
            desc.indexOffset = static_cast<uint32_t>(geometry.indices.size());
            desc.vertexOffset = 0;
            desc.vertexCount = static_cast<uint32_t>(geometry.positions.size());
            desc.lodOffset[0] = 0;
            desc.lodOffset[1] = static_cast<uint32_t>(bucket.size());

            geometry.indices.insert(geometry.indices.end(), bucket.begin(), bucket.end());
            geometry.meshes.push_back(desc);
            defaultMaterialIDs.push_back(materialID);
        }

        std::cout << "[mesh-check] " << objFilepath.filename().string()
            << " corners=" << checkedCorners
            << " uniqueVertices=" << geometry.positions.size()
            << " missingNormals=" << missingNormals
            << " missingUVs=" << missingUVs
            << " dedupMismatches=0" << '\n';

        recalculateBoundingBoxes(geometry);

        auto materials = rawMaterials
            | std::views::transform([](const auto& raw) {
            MaterialRecord rec{};
            copyToFixed(rec.name, raw.name);
            copyToFixed(rec.diffuseTexturePath, raw.diffuse_texname);
            rec.diffuseColor = { raw.diffuse[0],raw.diffuse[1],raw.diffuse[2] };
            rec.dissolve = raw.dissolve;
            rec.specularColor = { raw.specular[0],raw.specular[1],raw.specular[2] };
            rec.shininess = raw.shininess;
            return rec;
                }) | std::ranges::to<std::vector>();

        return BakedMesh{
            .geometry = std::move(geometry),
            .materials = std::move(materials),
            .defaultMaterialsIds = std::move(defaultMaterialIDs),
            .mtlBaseDir = mtlBaseDir
        };
    }

    std::expected<BakedMesh, std::string> loadOrBakeMesh(const std::filesystem::path& objFilepath) {
        const std::filesystem::path meshCachePath = objFilepath.string() + ".mesh";
        const std::filesystem::path matCachePath = objFilepath.string() + ".matCache";

        if (cacheIsFresh(objFilepath, matCachePath, meshCachePath)) {

            auto geometry = loadMeshData(meshCachePath);
            auto matCache = loadMaterialCache(matCachePath);

            if (geometry && matCache) {
                return BakedMesh{
                    .geometry = std::move(*geometry),
                    .materials = std::move(matCache->materials),
                    .defaultMaterialsIds = std::move(matCache->defaultMaterialIDs),
                    .mtlBaseDir = objFilepath.parent_path().string()
                };
            }
        }
        auto baked = bakeMeshFromObj(objFilepath);
        if (!baked) return baked;

        if (auto r = saveMeshData(meshCachePath, baked->geometry)) {}

        saveMaterialCache(matCachePath, MaterialCacheData{
                .materials = baked->materials,
                .defaultMaterialIDs = baked->defaultMaterialsIds
            });

        return baked;


    }
}