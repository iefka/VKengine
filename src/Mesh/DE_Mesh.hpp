#pragma once

#include<vulkan/vulkan.hpp>

#include<memory>
#include<vector>
#include<string>
#include<expected>
#include<filesystem>

#include"glm_config.hpp"
#include"DE_Memory.hpp"
#include"DE_GeometryHandle.hpp"
#include"DE_MeshBaker.hpp"

namespace de {
	class Device;
	class GeometryPool;

	class  Mesh final {
	public:
		
		struct Vertex {
			glm::vec4 position{ 0.f,0.f,0.f,1.f }; //ignore w
			glm::vec4 normal{ 0.f,0.f,0.f,0.f }; //ignore w
			glm::vec4 color{ 1.f,1.f,1.f,1.f };
			glm::vec2 uv{ 0.f,0.f};
			glm::vec2 _padding{};

			bool operator==(const Vertex&) const = default;

			static const std::vector<vk::VertexInputAttributeDescription> getAttributes() noexcept;
			static const std::vector <vk::VertexInputBindingDescription> getBindings()    noexcept;
		};

		struct SubMesh {
			uint32_t firstIndex{};
			uint32_t indexCount{};
			int32_t vertexOffset{};
		};

		struct LoadResult {
			std::unique_ptr<Mesh> mesh;
			std::vector<MaterialRecord> materials;

			std::vector<int32_t> defaultMaterialIDs;
			std::string mtlBaseDir;
		};


		// loading from file with subdivision into sub-mesh
		static std::expected<LoadResult, std::string>
			loadFromFile(const Device& device,
				GeometryPool& geometryPool,
				const std::filesystem::path& objFilepath);


		struct GridDesc {
			uint32_t resolutionU{ 64 };
			uint32_t resolutionV{ 64 };
		};

		static std::unique_ptr<Mesh> loadFromComputeShader(
			const Device& device,
			GeometryPool& geometryPool,
			const std::filesystem::path& computeShaderPath,
			const GridDesc& grid
		);


		void drawSubMesh(const vk::CommandBuffer& commandBuffer, uint32_t subMeshIndex,uint32_t firstInstance) const;
		vk::DrawIndexedIndirectCommand getIndirectCommand(uint32_t subMeshIndex, uint32_t firstInstance);


		const std::vector<SubMesh>& getSubMeshes() const noexcept { return subMeshes_; }

		~Mesh();

		Mesh(const Mesh&) = delete;
		Mesh& operator=(const Mesh&) = delete;
		Mesh(Mesh&&) = default;
		Mesh& operator=(Mesh&&) = default;

	private:
		explicit Mesh(const Device& device) : device_(device){}


		void dispatchMeshGeneration(const std::filesystem::path& computeShaderPath,
			const GridDesc& grid);
		static std::vector<uint32_t> generateGridIndices(uint32_t resU, uint32_t resV);

		const Device& device_;

		GeometryPool* pool_ = nullptr;
		GeometryHandle handle_{};

		std::vector<SubMesh> subMeshes_;
	} ;
}