#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"


#include"DE_Device.hpp"
#include "DE_Model.hpp"
#include"Utility/DE_Utility.hpp"
#include"Material.hpp"
#include"MaterialManager.hpp"

#include <iostream>
#include <unordered_map>

namespace std{
	template<>
	struct hash<de::Model::Vertex> {
		size_t operator() (de::Model::Vertex const& vertex) const {
			size_t seed = 0;
			de::utl::hashCombine(seed, vertex.position, vertex.normal, vertex.uv);
			return seed;
		}
	};
}

namespace de {

	Model::Model(const Device& device, const Builder& builder) : device_{ device } {

		material_ = builder.tempMaterialPtr;
		createVertexBuffer(builder.vertices);
		createIndexBuffer(builder.indices);
	}

	std::unique_ptr<Model> Model::createModelFromFile(const Device& device, const std::string& filepath, MaterialManager& materialManager)
	{
		Builder builder{};
		builder.loadModel(filepath,materialManager);
		std::cout <<"Vertex count : " << builder.vertices.size() << std::endl;
		return std::make_unique<Model>(device,builder);
	}

	void Model::Builder::loadModel(const std::string& filepath, MaterialManager& materialManager) {
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;

		if (!tinyobj::LoadObj(&attrib, &shapes, &materials,&warn, &err, filepath.c_str(), materialManager.getMaterialsPath().c_str())) {
			throw std::runtime_error(err);
		}
		
		vertices.clear();
		indices.clear();

		if (!materials.empty()) {
			tempMaterialPtr = materialManager.getMaterial(materials[0]);
		}else{
			tempMaterialPtr = materialManager.getDefaultMaterial();
		}

		std::unordered_map<Vertex, uint32_t> uniqueVertices{};
		for (const auto& shape : shapes) {
			
			for (const auto& index : shape.mesh.indices) {
				Vertex vertex{};
				if (index.vertex_index >= 0) {
					vertex.position = {
						attrib.vertices[3 * index.vertex_index + 0],
						attrib.vertices[3 * index.vertex_index + 1],
						attrib.vertices[3 * index.vertex_index + 2]
					};

				}

				if (index.normal_index >= 0) {
					vertex.normal = {
						attrib.normals[3 * index.normal_index + 0],
						attrib.normals[3 * index.normal_index + 1],
						attrib.normals[3 * index.normal_index + 2]
					};
				}

				if (index.texcoord_index >= 0) {
					vertex.uv = {
						attrib.texcoords[2 * index.texcoord_index + 0],
						attrib.texcoords[2 * index.texcoord_index + 1],
					};
				}
				
				auto [it, inserted] = uniqueVertices.try_emplace(vertex, static_cast<uint32_t>(
					vertices.size())
				);
				if (inserted) {
					vertices.push_back(vertex);
				}

				indices.push_back(it->second);
			}
		}
	}

	void Model::bind(const vk::CommandBuffer& commandBuffer) {
		commandBuffer.bindVertexBuffers(0, *vertexBuffer_->getBuffer(), { 0 });

		if (hasIndices) {
			commandBuffer.bindIndexBuffer(*indexBuffer_->getBuffer(), 0, vk::IndexType::eUint32);
		}
	}

	void Model::draw(const vk::CommandBuffer& commandBuffer){
		if(hasIndices){
			commandBuffer.drawIndexed(indexCount_, 1, 0, 0, 0);
		}
		else { 
			commandBuffer.draw(vertexCount_, 1, 0, 0); 
		}
	}

	const Material& Model::getMaterial() {
		return *material_;
	}

	void Model::createVertexBuffer(const std::vector<Vertex>& vertices) {

		//defining sizes
		vk::DeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
		vertexCount_ = vertices.size();

		//creating staging buffer to later copy
		de::MemoryUsageInfo stagingBufferInfo{
			 bufferSize,
			1,
			0,
			vk::BufferUsageFlagBits::eTransferSrc};

		Buffer stagingBuffer(device_, stagingBufferInfo);
		stagingBuffer.map();
		// copy data to staging buffer
		stagingBuffer.copyToBuffer(vertices,0);

		//creating device local vertex buffer
		de::MemoryUsageInfo vertexBufferInfo{
			 bufferSize,
			1,
			0,
			vk::BufferUsageFlagBits::eTransferDst|vk::BufferUsageFlagBits::eVertexBuffer,
			vk::MemoryPropertyFlagBits::eDeviceLocal};

		vertexBuffer_ = std::make_unique<Buffer>(device_, vertexBufferInfo);
		//copy data from staging buffer to vertex buffer
		device_.copyBuffer(*stagingBuffer.getBuffer(), *vertexBuffer_->getBuffer(), bufferSize);
	}

	void Model::createIndexBuffer(const std::vector<uint32_t>& indices) {
		// create buffer only if model have indices
		hasIndices = indices.size() > 0;

		if (!hasIndices) {
			return;
		}
		//defining sizes
		vk::DeviceSize bufferSize = sizeof(indices[0]) * indices.size();
		indexCount_ = indices.size();

		//creating staging buffer to later copy
		de::MemoryUsageInfo stagingBufferInfo{
			 bufferSize,
			1,
			0,
			vk::BufferUsageFlagBits::eTransferSrc };

		Buffer stagingBuffer(device_, stagingBufferInfo);
		stagingBuffer.map();
		stagingBuffer.copyToBuffer(indices,0);

		//creating device local index buffer
		de::MemoryUsageInfo indexBufferInfo{
			 bufferSize,
			1,
			0,
			vk::BufferUsageFlagBits::eTransferDst | vk::BufferUsageFlagBits::eIndexBuffer,
		vk::MemoryPropertyFlagBits::eDeviceLocal };

		indexBuffer_ = std::make_unique<Buffer>(device_, indexBufferInfo);
		//copy data from staging buffer to index buffer
		device_.copyBuffer(*stagingBuffer.getBuffer(), *indexBuffer_->getBuffer(), bufferSize);
	}

	const std::vector<vk::VertexInputBindingDescription> Model::Vertex::getBindings() noexcept {
		std::vector<vk::VertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = vk::VertexInputRate::eVertex;
		return bindingDescriptions;
	}

	const std::vector<vk::VertexInputAttributeDescription> Model::Vertex::getAttributes() noexcept {
		std::vector<vk::VertexInputAttributeDescription> attributeDescriptions{};

		attributeDescriptions.push_back({ 0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position) });
		attributeDescriptions.push_back({ 1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color) });
		attributeDescriptions.push_back({ 2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal) });
		attributeDescriptions.push_back({ 3, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, uv) });

		return attributeDescriptions;
	}
	
}
