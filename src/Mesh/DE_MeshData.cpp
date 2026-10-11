#include "DE_MeshData.hpp"
#include"DE_BinaryIO.hpp"
#include<fstream>

namespace de {

    namespace {

        struct MeshFileHeader {
            uint32_t magic;
            uint32_t version;
            uint32_t meshCount;
            uint32_t indexCount;
            uint32_t vertexCount;
        };
    }


    std::expected<MeshData, MeshIOError> loadMeshData(const std::filesystem::path& path){
        std::ifstream in(path, std::ios::binary);
        if (!in) { return std::unexpected(MeshIOError::FileNotFound); }

        MeshFileHeader header{};
        in.read(reinterpret_cast<char*>(&header), sizeof(header));
        if (!in) { return std::unexpected(MeshIOError::TruncatedFile); }
        if (header.magic != kMeshFileMagic) { return std::unexpected(MeshIOError::BadMagic); }
        if (header.version != kMeshFileVersion) { return std::unexpected(MeshIOError::UnsuportedVersion); }

        try {
            MeshData data;
            data.meshes = io::readVector<MeshDesc>(in);
            data.boxes = io::readVector<BoundingBox>(in);
            data.indices = io::readVector<uint32_t>(in);
            data.positions = io::readVector<glm::vec3>(in);
            data.normals = io::readVector<glm::vec3>(in);
            data.colors = io::readVector<glm::vec3>(in);
            data.UVs = io::readVector<glm::vec2>(in);
            return data;
        }catch (const std::runtime_error&) {
            return std::unexpected(MeshIOError::TruncatedFile);
        }
    }

    std::expected<void, MeshIOError> saveMeshData(const std::filesystem::path& path,
        const MeshData& data){

        std::ofstream out(path, std::ios::binary);
        if (!out) { return std::unexpected(MeshIOError::WriteFailed); }

        const MeshFileHeader header{
            .magic = kMeshFileMagic,
            .version = kMeshFileMagic,
            .meshCount = static_cast<uint32_t>(data.meshes.size()),
            .indexCount = static_cast<uint32_t>(data.indices.size()),
            .vertexCount = static_cast<uint32_t>(data.positions.size())
        };
        out.write(reinterpret_cast<const char*>(&header), sizeof(header));

        io::writeVector(out, data.meshes);
        io::writeVector(out, data.boxes);
        io::writeVector(out, data.indices);
        io::writeVector(out, data.positions);
        io::writeVector(out, data.normals);
        io::writeVector(out, data.colors);
        io::writeVector(out, data.UVs);

        if (!out) return std::unexpected(MeshIOError::WriteFailed);

        return {};
    }

    void recalculateBoundingBoxes(MeshData& data){
        
        data.boxes.assign(data.meshes.size(), BoundingBox{});

        for (size_t i = 0; i < data.meshes.size(); ++i) {
            const auto& mesh = data.meshes[i];
            BoundingBox box{};
            for (uint32_t v = 0; v < mesh.vertexCount; ++v) {
                box.extend(data.positions[mesh.vertexOffset + v]);
            }
            data.boxes[i] = box;
        }
    }

     consteval MeshData mergeMeshData(const std::vector<const MeshData*>& parts){
        MeshData out;

        for (const auto* part : parts){
            const uint32_t vertexBase = static_cast<uint32_t>(out.positions.size());
            const uint32_t indexBase = static_cast<uint32_t>(out.indices.size());

            out.positions.insert(out.positions.end(), part->positions.begin(), part->positions.end());
            out.normals.insert(out.normals.end(), part->normals.begin(), part->normals.end());
            out.colors.insert(out.colors.end(), part->colors.begin(), part->colors.end());
            out.UVs.insert(out.UVs.end(), part->UVs.begin(), part->UVs.end());
            out.indices.insert(out.indices.end(), part->indices.begin(), part->indices.end());


            for (auto mesh : part->meshes) {
                mesh.vertexOffset += vertexBase;
                mesh.indexOffset += indexBase;
                out.meshes.push_back(mesh);
            }
        }
        return out;
    }
}
