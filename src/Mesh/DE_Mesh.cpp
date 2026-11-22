#include"DE_Device.hpp"
#include"DE_GeometryPool.hpp"
#include"DE_Mesh.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Shaders.hpp"
#include"DE_Pipeline.hpp"
#include"DE_PushConstants.hpp"
#include<ranges>

namespace de {
    std::expected<Mesh::LoadResult, std::string> Mesh::loadFromFile(
        GeometryPool& geometryPool,
        const std::filesystem::path& objFilepath){

        auto baked = loadOrBakeMesh(objFilepath);

        if (!baked) { return std::unexpected(baked.error()); }

        const auto& geometry = baked->geometry;

        auto vertices = std::views::zip(
            geometry.positions, geometry.normals, geometry.colors, geometry.UVs)
            | std::views::transform([](const auto& components) {
                const auto& [position, normal, color, uv] = components;
                return Vertex{
                    .position = { position,1.f },
                    .normal = {normal,0.f},
                    .color = {color,1.f},
                    .uv = uv
                };
            }
            ) | std::ranges::to<std::vector>();

        auto mesh = std::unique_ptr<Mesh>(new Mesh());
        mesh->pool_ = &geometryPool;
        mesh->handle_ = geometryPool.upload(vertices, geometry.indices);

        mesh->subMeshes_ = geometry.meshes
            | std::views::transform([](const auto& desc) {
                return SubMesh{
                    .firstIndex = desc.indexOffset,
                    .indexCount = desc.getLodIndexCount(0),
                    .vertexOffset = static_cast<int32_t>(desc.vertexOffset)
                };
            }) | std::ranges::to<std::vector>();


        return LoadResult{
            .mesh = std::move(mesh),
            .materials = baked->materials,
            .defaultMaterialIDs = baked->defaultMaterialsIds,
            .mtlBaseDir = baked->mtlBaseDir
        };
    }

    std::unique_ptr<Mesh> Mesh::prepareForComputeMesh(
        GeometryPool& geometryPool,
        const GridDesc& grid){
        
        auto mesh = std::unique_ptr<Mesh>(new Mesh());

        const uint32_t vertexCount = grid.resolutionU * grid.resolutionV;
        auto indices = generateGridIndices(grid.resolutionU, grid.resolutionV);

        mesh->pool_ = &geometryPool;
        mesh->handle_ = geometryPool.reserveForCompute(vertexCount, indices);

        mesh->subMeshes_.push_back(
            SubMesh{
                .firstIndex = 0,
                .indexCount = static_cast<uint32_t>(indices.size()),
                .vertexOffset = 0
            }
        );

        return mesh;
    }

    void Mesh::drawSubMesh(const vk::CommandBuffer & commandBuffer, uint32_t subMeshIndex,uint32_t firstInstance) const{
        const auto& sub = subMeshes_.at(subMeshIndex);
        
        commandBuffer.drawIndexed(
            sub.indexCount,
            1,
            handle_.indexOffset + sub.firstIndex,
            static_cast<int32_t>(handle_.vertexOffset) + sub.vertexOffset,
            firstInstance
        );
    }

    vk::DrawIndexedIndirectCommand Mesh::getIndirectCommand(uint32_t subMeshIndex, uint32_t firstInstance)
    {
        const auto& sub = subMeshes_.at(subMeshIndex);

        return vk::DrawIndexedIndirectCommand{}
            .setFirstIndex(handle_.indexOffset + sub.firstIndex)
            .setFirstInstance(firstInstance)
            .setInstanceCount(1)
            .setVertexOffset(static_cast<int32_t>(handle_.vertexOffset) + sub.vertexOffset)
            .setIndexCount(sub.indexCount);
    }

    std::vector<uint32_t> Mesh::generateGridIndices(uint32_t resU, uint32_t resV){
        std::vector<uint32_t> indices;
        indices.reserve(static_cast<size_t>(resU - 1) * (resV - 1) * 6);

        auto vertexIndex = [resU](uint32_t u, uint32_t v) {return v * resU + u; };

        for (uint32_t v = 0; v < resV - 1; ++v) {
            for (uint32_t u = 0; u < resU - 1; ++u) {
                const uint32_t topLeft = vertexIndex(u, v);
                const uint32_t topRight = vertexIndex(u + 1, v);
                const uint32_t bottomLeft = vertexIndex(u, v + 1);
                const uint32_t bottomRight = vertexIndex(u + 1, v + 1);

                indices.push_back(topLeft);
                indices.push_back(bottomLeft);
                indices.push_back(topRight);

                indices.push_back(topRight);
                indices.push_back(bottomLeft);
                indices.push_back(bottomRight);
            }
        }
        return indices;
    }

    const std::vector<vk::VertexInputBindingDescription> Mesh::Vertex::getBindings() noexcept {
        return { vk::VertexInputBindingDescription{
            0,
            sizeof(Vertex),
            vk::VertexInputRate::eVertex
            } 
        };
    }

    const std::vector<vk::VertexInputAttributeDescription> Mesh::Vertex::getAttributes() noexcept {

        return {
            {0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, position)},
            {1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)},  
            {2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, normal)},  
            {3, 0, vk::Format::eR32G32Sfloat,       offsetof(Vertex, uv)},
        };

    }
    Mesh::~Mesh() {
        if (pool_) {
            pool_->free(handle_);
        }
    }
}
