#include"DE_Device.hpp"
#include"DE_GeometryPool.hpp"
#include"DE_Mesh.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Shaders.hpp"
#include"DE_Pipeline.hpp"
#include"DE_PushConstants.hpp"
#include<iostream>

#include<ranges>

namespace de {
    std::expected<Mesh::LoadResult, std::string> Mesh::loadFromFile(
        const Device& device,
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

        auto mesh = std::unique_ptr<Mesh>(new Mesh(device));
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

        std::cout << objFilepath.filename().string()
            << ": vertices = " << vertices.size()
            << ", triangles = " << geometry.indices.size() / 3
            << '\n';

        return LoadResult{
            .mesh = std::move(mesh),
            .materials = baked->materials,
            .defaultMaterialIDs = baked->defaultMaterialsIds,
            .mtlBaseDir = baked->mtlBaseDir
        };
    }

    std::unique_ptr<Mesh> Mesh::loadFromComputeShader(
        const Device& device,
        GeometryPool& geometryPool,
        const std::filesystem::path& computeShaderPath, 
        const GridDesc& grid){
        
        auto mesh = std::unique_ptr<Mesh>(new Mesh(device));

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

        mesh->dispatchMeshGeneration(computeShaderPath, grid);
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


    void Mesh::dispatchMeshGeneration(const std::filesystem::path & computeShaderPath, const GridDesc & grid){
        struct PushData {
            uint32_t resolutionU{};
            uint32_t resolutionV{};
            float time{};
            float _pad{};
        };

        auto descriptorPool = DescriptorPool::Builder{ device_ }
            .setMaxSets(1)
            .addPoolSize(vk::DescriptorType::eStorageBuffer, 1)
            .build();


        auto descriptorLayout = DescriptorSetLayout::Builder{ device_ }
            .addBinding(0, vk::DescriptorType::eStorageBuffer, vk::ShaderStageFlagBits::eCompute)
            .build();

        auto bufferInfo = pool_->getVertexRangeDescriptor(handle_.vertexOffset,handle_.vertexCount);
        auto descriptorSet = DescriptorWriter{ device_, descriptorPool, descriptorLayout}
            .writeBuffer(0, &bufferInfo)
            .build();

        
        PushData pushData{
            .resolutionU = grid.resolutionU,
            .resolutionV = grid.resolutionV,
            .time = 0.f
        };
        PushConstant<PushData> pushConstant(vk::ShaderStageFlagBits::eCompute);
        pushConstant.setValue(pushData);

        Shader computeShader(device_, computeShaderPath);

        const auto computePipelineInfo = ComputePipeline::Builer{}
            .setPushConstantRanges({ pushConstant.getRange() })
            .addDsLayout(descriptorLayout.getLayout())
            .setShaderStage(computeShader.getStageInfo(vk::ShaderStageFlagBits::eCompute));
        ComputePipeline computePipeline(device_, computePipelineInfo);


        device_.executeSingleCommand([&](const vk::CommandBuffer& cmd) {
            const auto& pipeline = *computePipeline.getPipeline();
            const auto& pipeLayout = *computePipeline.getLayout();

            cmd.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
            cmd.bindDescriptorSets(vk::PipelineBindPoint::eCompute, pipeLayout, 0, descriptorSet, {});
            cmd.pushConstants(pipeLayout, vk::ShaderStageFlagBits::eCompute,0,sizeof(pushData),&pushData);

            const uint32_t groupX = (grid.resolutionU + 15) / 16;
            const uint32_t groupY = (grid.resolutionV + 15) / 16;
            cmd.dispatch(groupX, groupY, 1);

            const auto barier = vk::BufferMemoryBarrier{}
                .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
                .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead)
                .setBuffer(pool_->getVertexBufferHandle())
                .setOffset(static_cast<vk::DeviceSize>(handle_.vertexOffset) * sizeof(Vertex))
                .setSize(static_cast<vk::DeviceSize>(handle_.vertexCount) * sizeof(Vertex))
                .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
                .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);

            cmd.pipelineBarrier(
                vk::PipelineStageFlagBits::eComputeShader,
                vk::PipelineStageFlagBits::eVertexInput,
                {},
                {},
                barier,
                {}
            );
        });
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
        std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};

        return {
            {0, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, position)},
            {1, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)},   // было normal
            {2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, normal)},  // было color
            {3, 0, vk::Format::eR32G32Sfloat,       offsetof(Vertex, uv)},
        };

        return attributeDescriptions;
    }
    Mesh::~Mesh() {
        if (pool_) {
            pool_->free(handle_);
        }
    }
}
