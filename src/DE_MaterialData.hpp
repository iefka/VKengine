#pragma once

#include "glm_config.hpp"
#include <cstdint>

namespace de {
    inline constexpr uint32_t kInvalidTextureIndex = 0xFFFFFFFFu;
    struct GPUMaterialData {
        glm::vec4 diffuseColor{ 0.6f, 0.6f, 0.6f, 1.f };   // rgb = Kd, a = dissolve/alpha (d/Tr)
        glm::vec4 specularColor{ 1.f, 1.f, 1.f, 512.f };    // rgb = Ks, a = shininess (Ns)

        uint32_t diffuseTexIndex = kInvalidTextureIndex;
        uint32_t normalTexIndex = kInvalidTextureIndex;
        uint32_t _pad0 = 0;
        uint32_t _pad1 = 0;
    };

    static_assert(sizeof(GPUMaterialData) == 48,
        "GPUMaterialData must stay 48 bytes - this is the std430 stride the "
        "shader (fragment.frag MaterialBuffer.materials[]) assumes. If you "
        "add/remove fields, update both sides and keep it a multiple of 16.");

} // namespace de

