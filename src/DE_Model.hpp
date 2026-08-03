#ifndef _DE_MODEL_
#define _DE_MODEL_

#include<vulkan/vulkan.hpp>

#include<memory>
#include<vector>
#include<string>

#include"DE_Memory.hpp"
#include"glm_config.hpp"

namespace de {
	class Device;

	class Model {
	public:
		struct Vertex {
		public:
			glm::vec3 position{};
			glm::vec3 color{0.1f,0.5f,0.8f};
			glm::vec3 normal{};
			glm::vec2 uv{};


			bool operator ==(const Vertex& other) const {
				return position == other.position &&
					   color == other.color &&
					   normal == other.normal &&
					   uv == other.uv;
			}

			static const std::vector<vk::VertexInputAttributeDescription> getAttributes() noexcept;
			static const std::vector <vk::VertexInputBindingDescription> getBindings()   noexcept;
		private:
			inline static std::vector<vk::VertexInputAttributeDescription> vertexAtributes;
			inline static std::vector <vk::VertexInputBindingDescription> vertexBindings;
		};
		struct Builder {
			std::vector<Vertex> vertices{};
			std::vector<uint32_t> indices{};

			void loadModel(const std::string& filepath);
		};

		Model(const Device& device, const Builder& builder);
		static std::unique_ptr<Model> createModelFromFile(const Device& device, const std::string& filepath);

		void bind(const vk::CommandBuffer& commandBuffer);
		void draw(const vk::CommandBuffer& commandBuffer);

	private:
		void createVertexBuffer(const std::vector<Vertex>& vertices);
		void createIndexBuffer(const std::vector<uint32_t>& indices);

		const Device& device_;
		std::unique_ptr<Buffer> vertexBuffer_;
		uint32_t vertexCount_;

		bool hasIndices;
		std::unique_ptr<Buffer> indexBuffer_;
		uint32_t indexCount_;
	};
}

#endif