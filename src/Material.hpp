#pragma once

#include "Texture.hpp"

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

namespace de {

    class Material {
    public:
        struct FrameBinding {
            uint32_t materialIndex{};
        };

        // Used by MaterialManager for ordinary file-backed materials.
        Material(std::shared_ptr<ITextureSource> texture, uint32_t materialIndex)
            : texture_{ std::move(texture) }, materialIndex_{ materialIndex } {}

        // Used for materials with a different GPU material index per frame.
        // The underlying compute textures must be kept alive by their owner.
        explicit Material(std::vector<FrameBinding> frames)
            : frames_{ std::move(frames) } {
            if (frames_.empty()) {
                throw std::invalid_argument("Material: no frame bindings");
            }
        }

        uint32_t getMaterialIndex(uint32_t frameIndex) const {
            return frames_.empty() ? materialIndex_ : frames_.at(frameIndex).materialIndex;
        }

        // Existing callers without a frame index remain valid for static materials.
        uint32_t getMaterialIndex() const {
            if (!frames_.empty()) {
                throw std::logic_error("Material: frame index required");
            }
            return materialIndex_;
        }

    private:
        std::shared_ptr<ITextureSource> texture_;
        uint32_t materialIndex_ = 0;
        std::vector<FrameBinding> frames_;
    };

} // namespace de
