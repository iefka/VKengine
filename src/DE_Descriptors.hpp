#ifndef _DE_DESCRIPTORS_
#define _DE_DESCRIPTORS_

#include <vulkan/vulkan.hpp>
#include<vector>
#include<unordered_map>

namespace de{
	class Device;

	class DescriptorSetLayout {
	public:
		class Builder {
		public:
			explicit Builder(const Device& device);
			Builder& addBinding(uint32_t binding,
				vk::DescriptorType descriptorType,
				vk::ShaderStageFlagBits stageFlag, uint32_t count = 1 );

			DescriptorSetLayout build();
		private:
			const Device& device_;
			std::unordered_map<int, vk::DescriptorSetLayoutBinding> bindings_;
		};

		DescriptorSetLayout(const Device& device, std::unordered_map<int, vk::DescriptorSetLayoutBinding> bindings);

		~DescriptorSetLayout() = default;

		DescriptorSetLayout(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
		DescriptorSetLayout(DescriptorSetLayout&&) = default;
		DescriptorSetLayout& operator=(DescriptorSetLayout&&) = default;

		const vk::DescriptorSetLayout& getLayout() const noexcept { return *descriptorSetLayout_; }
		const std::unordered_map<int, vk::DescriptorSetLayoutBinding>& getBinding() const noexcept { return bindings_;}


	private:
		vk::UniqueDescriptorSetLayout descriptorSetLayout_;
		std::unordered_map<int, vk::DescriptorSetLayoutBinding> bindings_;
	};


	class DescriptorPool{
	public:
		
		class Builder{
		public:
			explicit Builder(const Device& device);

			Builder& addPoolSize(vk::DescriptorType descriptorType, uint32_t size);
			Builder& setPoolFlags(vk::DescriptorPoolCreateFlags flags);
			Builder& setMaxSets(uint32_t maxSets);

			DescriptorPool build() const;
		private:
			const Device& device_;
			std::vector<vk::DescriptorPoolSize> poolSizes_;
			vk::DescriptorPoolCreateFlags poolFlags_ = {};
			uint32_t maxSets_ = 1000;
		};

		DescriptorPool(const Device& device, uint32_t maxSets, vk::DescriptorPoolCreateFlags flags,
			const std::vector<vk::DescriptorPoolSize> poolSizes);

		~DescriptorPool() = default;

		DescriptorPool(const DescriptorPool&) = delete;
		DescriptorPool& operator=(const DescriptorPool&) = delete;
		DescriptorPool(DescriptorPool&&) = default;
		DescriptorPool& operator=(DescriptorPool&&) = default;

		vk::UniqueDescriptorSet allocate(const DescriptorSetLayout& descriptorSetLayout) const;


	private:
		const Device& device_;
		vk::UniqueDescriptorPool descriptorPool_;
	};

	


	class DescriptorWriter {
	public:
		DescriptorWriter(const Device& device,const DescriptorPool& descriptorPool, const DescriptorSetLayout& descriptorLayout)
			:pool_(descriptorPool), layout_(descriptorLayout), device_(device) {}
		
		DescriptorWriter& writeBuffer(uint32_t binding, const std::vector <vk::DescriptorBufferInfo>& imageInfo);
		DescriptorWriter& writeImage(uint32_t binding, const std::vector <vk::DescriptorImageInfo>& imageInfo);
		vk::UniqueDescriptorSet build();
		void overwrite(const vk::UniqueDescriptorSet& set);

	private:
		const Device& device_;
		const DescriptorPool& pool_;
		const DescriptorSetLayout& layout_;
		std::vector<vk::WriteDescriptorSet> writes_;
	};
}

#endif