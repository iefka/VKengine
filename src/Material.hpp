#pragma once
#include"Texture.hpp"

namespace de {
	class Material {
	public:

		Material(std::shared_ptr<ITextureSource> texture, uint32_t materialIndex)
			: texture_{ std::move(texture) }
			, materialIndex_{ materialIndex } {}

		uint32_t getMaterialIndex() const noexcept {
			return materialIndex_;
		}

	private:

		std::shared_ptr<ITextureSource> texture_;
		uint32_t materialIndex_;
	};
}
