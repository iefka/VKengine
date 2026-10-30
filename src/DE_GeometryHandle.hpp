#pragma once

#include<cstdint>

namespace de{

	struct GeometryHandle {
		uint32_t vertexOffset = 0;
		uint32_t vertexCount = 0;
		uint32_t indexOffset = 0;
		uint32_t indexCount = 0;
	};
}