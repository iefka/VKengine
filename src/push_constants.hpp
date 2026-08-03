#pragma once

#include <vulkan/vulkan.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include <type_traits>
#include <cassert>

namespace de {

struct PushConstantDesc {
	std::string name;
	vk::ShaderStageFlags stageFlags{};
	uint32_t offset{};
	uint32_t size{};

	PushConstantDesc() = default;
	PushConstantDesc(const std::string& n, vk::ShaderStageFlags s, uint32_t o, uint32_t sz)
		: name(n), stageFlags(s), offset(o), size(sz) {}

	vk::PushConstantRange toRange() const {
		return vk::PushConstantRange{}
			.setStageFlags(stageFlags)
			.setOffset(offset)
			.setSize(size);
	}
};

class PushConstantsRegistry {
public:
	PushConstantsRegistry() = default;

	void add(const PushConstantDesc& desc) {
		ranges_.push_back(desc.toRange());
		descs_.emplace(desc.name, desc);
	}

	const std::vector<vk::PushConstantRange>& ranges() const noexcept { return ranges_; }

	const PushConstantDesc* get(const std::string& name) const noexcept {
		auto it = descs_.find(name);
		if (it == descs_.end()) return nullptr;
		return &it->second;
	}

private:
	std::vector<vk::PushConstantRange> ranges_;
	std::unordered_map<std::string, PushConstantDesc> descs_;
};

// type-safe push helper
template<typename T>
inline void pushConstants(const vk::CommandBuffer& cb, const vk::PipelineLayout& layout, const PushConstantDesc& desc, const T& value) {
	static_assert(std::is_trivially_copyable<T>::value, "Push constants require trivially copyable/POD types");
	assert(sizeof(T) <= desc.size && "push data larger than declared push constant size");

	const auto pushInfo = vk::PushConstantsInfo{}
		.setLayout(layout)
		.setStageFlags(desc.stageFlags)
		.setOffset(desc.offset)
		.setSize(static_cast<uint32_t>(sizeof(T)))
		.setPValues(&value);

	cb.pushConstants2(pushInfo);
}

} // namespace de
