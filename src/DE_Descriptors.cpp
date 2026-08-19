#include "DE_Device.hpp"
#include "DE_Descriptors.hpp"
#include <algorithm>
#include <utility>


namespace de{

	//---Descriptor Pool---
	DescriptorPool::Builder::Builder(const Device& device) : device_(device){}

	DescriptorPool::Builder& 
		DescriptorPool::Builder::addPoolSize(vk::DescriptorType descriptorType, uint32_t size){

		poolSizes_.push_back({descriptorType,size});
		return *this;
	}
	DescriptorPool::Builder& 
		DescriptorPool::Builder::setPoolFlags(vk::DescriptorPoolCreateFlags flags){

		poolFlags_ = flags;
		return *this;
	}
	DescriptorPool::Builder& 
		DescriptorPool::Builder::setMaxSets(uint32_t maxSets){

		maxSets_ = maxSets;
		return *this;
	}

	DescriptorPool DescriptorPool::Builder::build() const{

		return DescriptorPool(device_, maxSets_, poolFlags_, poolSizes_);
	}

	DescriptorPool::DescriptorPool(const Device& device, uint32_t maxSets, vk::DescriptorPoolCreateFlags flags,
		const std::vector<vk::DescriptorPoolSize> poolSizes) :device_(device){

		const auto& logicalDevice = device_.getLogicalDevice();

		const auto desciptorPoolInfo = vk::DescriptorPoolCreateInfo{}
			.setFlags(flags)
			.setMaxSets(maxSets)
			.setPoolSizes(poolSizes);

		descriptorPool_ = logicalDevice.createDescriptorPoolUnique(desciptorPoolInfo);

	}

	vk::DescriptorSet DescriptorPool::allocate(const DescriptorSetLayout& descriptorSetLayout) const {

		const auto& logicalDevice = device_.getLogicalDevice();
		
		const auto descriptorAllocInfo = vk::DescriptorSetAllocateInfo{}
			.setDescriptorPool(*descriptorPool_)
			.setDescriptorSetCount(1)
			.setSetLayouts(descriptorSetLayout.getLayout());

		 auto sets = logicalDevice.allocateDescriptorSets(descriptorAllocInfo);
		  return std::move(sets[0]);
	}

	//---DescriptorSetLayout--- 

		DescriptorSetLayout::Builder::Builder(const Device& device) : device_(device){}

		DescriptorSetLayout::Builder&
			DescriptorSetLayout::Builder::addBinding(uint32_t binding,
				vk::DescriptorType descriptorType,
				vk::ShaderStageFlagBits stageFlag, uint32_t count){

			if (bindings_.count(binding) != 0) {
				throw std::runtime_error("Binding already added");
			}

			const auto descriptorBinding = vk::DescriptorSetLayoutBinding{}
				.setBinding(binding)
				.setDescriptorType(descriptorType)
				.setDescriptorCount(count)
				.setStageFlags(stageFlag);
				
			bindings_[binding] = descriptorBinding;

			return *this;
		}

		// use move senematics, dont use builder after build
		DescriptorSetLayout DescriptorSetLayout::Builder::build(){

			return DescriptorSetLayout(device_,std::move(bindings_));
		}

		DescriptorSetLayout::DescriptorSetLayout(const Device& device,
			std::unordered_map<uint32_t, vk::DescriptorSetLayoutBinding> bindings) : bindings_(std::move(bindings)){

			const auto& logicalDevice = device.getLogicalDevice();

			std::vector<vk::DescriptorSetLayoutBinding> bindingList;
			bindingList.reserve(bindings_.size());

			for (const auto& [binding, layoutBinding] : bindings_) {
				bindingList.push_back(layoutBinding);
			}

			std::sort(bindingList.begin(), bindingList.end(),
				[](const auto& a, const auto& b) {
					return a.binding < b.binding;
				});

			const auto descriptorLayoutInfo = vk::DescriptorSetLayoutCreateInfo{}
				.setBindings(bindingList);

			descriptorSetLayout_ = logicalDevice.createDescriptorSetLayoutUnique(descriptorLayoutInfo);
		}
		void DescriptorWriter::overwrite(const vk::DescriptorSet& set){
			const auto& logicalDevice = device_.getLogicalDevice();
			for (auto&& write : writes_) {
				write.setDstSet(set);
			}
			logicalDevice.updateDescriptorSets(writes_,{});
		}

		DescriptorWriter& DescriptorWriter::writeBuffer(uint32_t binding, vk::DescriptorBufferInfo* bufferInfo){

			const auto& bindingList = layout_.getBinding();

			auto it = bindingList.find(binding);
			if (it == bindingList.end()) {
				throw std::runtime_error("binding not found in layout");
			}


			const auto write = vk::WriteDescriptorSet{}
				.setDstBinding(binding)
				.setDescriptorCount(1)
				.setDescriptorType(it->second.descriptorType)
				.setPBufferInfo(bufferInfo);

			writes_.push_back(write);

			return *this;
		}

		DescriptorWriter& DescriptorWriter::writeImage(uint32_t binding, vk::DescriptorImageInfo* imageInfo){
			const auto& bindingList = layout_.getBinding();

			auto it = bindingList.find(binding);
			if (it == bindingList.end()) {
				throw std::runtime_error("binding not found in layout");
			}


			const auto write = vk::WriteDescriptorSet {}
				.setDstBinding(binding)
				.setDescriptorCount(1)
				.setDescriptorType(it->second.descriptorType)
				.setPImageInfo(imageInfo);

			writes_.push_back(write);

			return *this;
		}

		vk::DescriptorSet DescriptorWriter::build(){

			auto outSet = pool_.allocate(layout_);
			overwrite(outSet);
			return outSet;
		}
		
}
