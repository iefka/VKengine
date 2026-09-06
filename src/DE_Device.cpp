//std
#include<algorithm>
#include<unordered_map>
//my
#include"DE_CommandBuffers.hpp"
#include"DE_Instance.hpp"
#include "DE_Device.hpp"


namespace de{

	Device::Device(Instance& instance, std::optional<vk::SurfaceKHR> surface, const std::vector<const char*>& extensions,
		const std::vector<const char*>& layers) :instance_{instance}, extensions_{extensions}, layers_{layers} {
		vk::Instance aInstance = instance_.getInstance();

		//create physical device
		const auto physicalDevices = aInstance.enumeratePhysicalDevices();
		if (physicalDevices.empty()){
			throw std::runtime_error("No Vulkan device found");
		}
		physicalDevice_ = pickDiscreteGpu(physicalDevices);

		//get queue families
		const auto queueFamilies = physicalDevice_.getQueueFamilyProperties();

		

		//request family index for two queue graphics and presentation
		queues_.resize(2);
		queues_[0].queuFamilyIndex = pickSuitableQueueFamily(queueFamilies, vk::QueueFlagBits::eGraphics);
		queues_[1].queuFamilyIndex = pickSuitableQueueFamily(queueFamilies,
			vk::QueueFlagBits::eGraphics, surface);
		
		uLogicalDevice_ = createLogicalDevice();

		//creating queue
		queues_[0].queue = uLogicalDevice_.get().getQueue(queues_[0].queuFamilyIndex,0);
		queues_[1].queue = uLogicalDevice_.get().getQueue(queues_[1].queuFamilyIndex,1);

		transientCommandPool_ = std::make_unique<CommandPool>(
			*this,
			getGraphicsQueue().queuFamilyIndex,
			vk::CommandPoolCreateFlagBits::eTransient);
	}

	void Device::copyBuffer(const vk::Buffer& srcBuffer, const vk::Buffer& dstBuffer, vk::DeviceSize size) const{
		auto cmd = beginSingleCommand();

		const auto& commandBuffer = cmd.handle(0);

		const auto BufferRegion = vk::BufferCopy{}
			.setDstOffset(0)
			.setSrcOffset(0)
			.setSize(size);
			
		commandBuffer.copyBuffer(srcBuffer,dstBuffer,BufferRegion);

		endSingleCommand(commandBuffer);
	}

	void Device::copyBufferToImage(const vk::Buffer& srcBuffer,
		const vk::Image& dstImage,
		uint32_t width, 
		uint32_t height) const{
		auto cmd = beginSingleCommand();

		const auto& commandBuffer = cmd.handle(0);

		const auto imageSubresource = vk::ImageSubresourceLayers{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setMipLevel(0);

		const auto region = vk::BufferImageCopy{}
			.setBufferRowLength(0)
			.setBufferOffset(0)
			.setBufferImageHeight(0)
			.setImageOffset(0)
			.setImageExtent(vk::Extent3D{ width,height,1 })
			.setImageSubresource(imageSubresource);

		commandBuffer.copyBufferToImage(srcBuffer, dstImage, vk::ImageLayout::eTransferDstOptimal, region);

		endSingleCommand(commandBuffer);


	}

	void Device::transitionImageLayout(vk::Image image, vk::Format format,
		vk::ImageLayout oldLayout, vk::ImageLayout newLayout) const {
	
		auto cmd = beginSingleCommand();
		const auto& commandBuffer = cmd.handle(0);

		vk::PipelineStageFlags sourceStage{}, destenationStage{};

		const auto imageSubresource = vk::ImageSubresourceRange{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(1)
			.setBaseMipLevel(0)
			.setLevelCount(1);

		 auto imageBarier = vk::ImageMemoryBarrier{}
			.setOldLayout(oldLayout)
			.setNewLayout(newLayout)
			.setDstQueueFamilyIndex(vk::QueueFamilyIgnored)
			.setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
			.setImage(image)
			.setSubresourceRange(imageSubresource);

		 if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eTransferDstOptimal) {
			 imageBarier.srcAccessMask  = vk::AccessFlagBits::eNone;
			 imageBarier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;

			 sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			 destenationStage = vk::PipelineStageFlagBits::eTransfer;
		 }else if (oldLayout == vk::ImageLayout::eTransferDstOptimal && newLayout == vk::ImageLayout::eShaderReadOnlyOptimal) {
			 imageBarier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
			 imageBarier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

			 sourceStage = vk::PipelineStageFlagBits::eTransfer;
			 destenationStage = vk::PipelineStageFlagBits::eFragmentShader;
		 }else {
			 throw std::runtime_error("unsuported image layout transition");
		 }


		commandBuffer.pipelineBarrier(
			sourceStage,
			destenationStage,
			vk::DependencyFlags{},
			nullptr,
			nullptr,
			imageBarier);

		endSingleCommand(commandBuffer);
	}

	CommandBuffer Device::beginSingleCommand() const
	{
		auto cmd = transientCommandPool_->allocate(vk::CommandBufferLevel::ePrimary, 1);

		const auto cmdBufferBeginInfo = vk::CommandBufferBeginInfo{}
		 .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit);

		cmd.handle(0).begin(cmdBufferBeginInfo);

		return cmd;
	}

	void Device::endSingleCommand(const vk::CommandBuffer& commandBuffer) const{
		
		commandBuffer.end();
		const auto submitInfo = vk::SubmitInfo{}
			.setCommandBufferCount(1)
			.setPCommandBuffers(&commandBuffer);

		getGraphicsQueue().queue.submit(submitInfo);
		getGraphicsQueue().queue.waitIdle();
	}

	vk::PhysicalDevice Device::pickDiscreteGpu(const std::vector<vk::PhysicalDevice>& devices){

		auto it = std::find_if(devices.begin(), devices.end(), [](vk::PhysicalDevice gpu){
			return gpu.getProperties().deviceType ==
				vk::PhysicalDeviceType::eDiscreteGpu;
		});

		if (it == devices.end()){
			throw std::runtime_error("No discrete Gpu found");
		}

		return *it;
	}


	uint32_t Device::pickSuitableQueueFamily(const std::vector<vk::QueueFamilyProperties>& queueFamilies,
		vk::QueueFlags requiredFlags, std::optional<vk::SurfaceKHR> surface) {

		uint32_t index{0};

		for (const auto& q : queueFamilies) {

			if (surface.has_value() &&
				!physicalDevice_.getSurfaceSupportKHR(index, *surface)) {
				continue;
			}
			if ((q.queueFlags & requiredFlags) == requiredFlags) {
				return index;
			}
			++index;
		}
		throw std::runtime_error("no suitable queue family found");
	}

	vk::UniqueDevice Device::createLogicalDevice(){


		std::unordered_map<uint32_t, uint32_t> familyIndexCount;

		for (const auto& qf : queues_) {
			familyIndexCount[qf.queuFamilyIndex]++;
		}

		vk::PhysicalDeviceFeatures deviceFeatures{};
		physicalDevice_.getFeatures(&deviceFeatures);

		deviceFeatures;

		std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos{};

		std::vector<std::vector<float>> prioritiesStorage;
		for (const auto& [familyIndex,count] : familyIndexCount) {
			prioritiesStorage.emplace_back(count, 1.0f);
			queueCreateInfos.emplace_back(vk::DeviceQueueCreateInfo{}
				.setQueueFamilyIndex(familyIndex)
				.setQueueCount(count)
				.setQueuePriorities(prioritiesStorage.back())
			);
		}
		
		using FeaturesChain = vk::StructureChain <vk::PhysicalDeviceFeatures2>;
		FeaturesChain featuresChainCheck;
		FeaturesChain featuresChainEnable;

		physicalDevice_.getFeatures2(&featuresChainCheck.get<vk::PhysicalDeviceFeatures2>());

		if (featuresChainCheck.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy == false) {
			throw std::runtime_error("anisontropy feture is not suported");
		}

		featuresChainEnable.get<vk::PhysicalDeviceFeatures2>().features.samplerAnisotropy = true;

		const auto deviceCreateInfo = vk::DeviceCreateInfo{}
			.setPNext(&featuresChainEnable)
			.setQueueCreateInfos(queueCreateInfos)
			.setPEnabledExtensionNames(extensions_)
			.setPEnabledLayerNames(layers_);


		return physicalDevice_.createDeviceUnique(deviceCreateInfo);
	}
	
	
}
