#pragma once
#include"Texture.hpp"

namespace de {
	class Material {
	public:

		struct materialData {
			Texture& texture;
		};

		// later with more data fields better create builder
		Material(std::shared_ptr<Texture> texture,vk::DescriptorSet descriptor):
			texture_{texture}
			,descriptor_{std::move(descriptor)}{}


		materialData getMaterialData() {
			return materialData{
				*texture_
			};
		}


		vk::DescriptorSet getDescriptor() const noexcept {
			return descriptor_;
		}

	private:
		vk::DescriptorSet descriptor_;
		std::shared_ptr <Texture> texture_;
	};
}