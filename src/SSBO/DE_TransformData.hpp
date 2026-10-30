#pragma once
#include"glm_config.hpp"

namespace de {

	struct GPUTransformData {
		glm::mat4 modelMatrix{ 1.f };
		glm::mat4 normalMatrix{};
		uint32_t materialIndex{ 0 };
		uint32_t _pad[3]{};
	};
}