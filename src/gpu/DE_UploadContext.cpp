#include"DE_UploadContext.hpp"

#include"DE_Device.hpp"

#include<limits>

namespace de{
    namespace{
        constexpr vk::DeviceSize kStagingAlignment = 16;

        vk::DeviceSize alignUp(vk::DeviceSize value, vk::DeviceSize alignment) noexcept{
            return (value + alignment - 1) / alignment * alignment;
        }
    }

    UploadContext::UploadContext(const Device& device, vk::DeviceSize stagingCapacity)
        :device_{device}
        ,stagingCapacity_{alignUp(stagingCapacity, kStagingAlignment)}{

        DE_ASSERT(stagingCapacity_ > 0);

        staging_ = std::make_unique<Buffer>(device_, MemoryUsageInfo{
            stagingCapacity_, 1, 0, vk::BufferUsageFlagBits::eTransferSrc});
        staging_->map();                    

        pool_ = device_.getLogicalDevice().createCommandPoolUnique(
            vk::CommandPoolCreateInfo{}
            .setFlags(vk::CommandPoolCreateFlagBits::eResetCommandBuffer
                | vk::CommandPoolCreateFlagBits::eTransient)
            .setQueueFamilyIndex(device_.getGraphicsQueue().queuFamilyIndex));

        cmd_ = device_.getLogicalDevice().allocateCommandBuffers(
            vk::CommandBufferAllocateInfo{}
            .setCommandPool(*pool_)
            .setLevel(vk::CommandBufferLevel::ePrimary)
            .setCommandBufferCount(1)).front();

        fence_ = device_.getLogicalDevice().createFenceUnique(vk::FenceCreateInfo{});
    }

    UploadContext::~UploadContext(){
    }

    void UploadContext::begin(){
        if(recording_){                     
            DE_ASSERT(!recording_);
            flush();                        
        }

        cmd_.begin(vk::CommandBufferBeginInfo{}
            .setFlags(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));
        recording_ = true;
    }

    void UploadContext::flush(){
        if(!recording_){
            return;
        }

        if(!writtenBuffers_.empty()){
 
            const auto barrier = vk::MemoryBarrier{}
                .setSrcAccessMask(vk::AccessFlagBits::eTransferWrite)
                .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead
                    | vk::AccessFlagBits::eIndexRead
                    | vk::AccessFlagBits::eUniformRead
                    | vk::AccessFlagBits::eShaderRead);

            cmd_.pipelineBarrier(
                vk::PipelineStageFlagBits::eTransfer,
                vk::PipelineStageFlagBits::eVertexInput
                    | vk::PipelineStageFlagBits::eVertexShader
                    | vk::PipelineStageFlagBits::eFragmentShader
                    | vk::PipelineStageFlagBits::eComputeShader,
                vk::DependencyFlags{},
                barrier, nullptr, nullptr);
        }

        cmd_.end();

        const auto& queue = device_.getGraphicsQueue().queue;
        queue.submit(vk::SubmitInfo{}.setCommandBuffers(cmd_), *fence_);

        const auto& logicalDevice = device_.getLogicalDevice();
        constexpr uint64_t kForever = std::numeric_limits<uint64_t>::max();
        while(logicalDevice.waitForFences(*fence_, vk::True, kForever) == vk::Result::eTimeout){}
        logicalDevice.resetFences(*fence_);

        cmd_.reset(vk::CommandBufferResetFlagBits::eReleaseResources);

        writtenBuffers_.clear();
        retired_.clear();                   
        stagingUsed_ = 0;                
        recording_ = false;
    }

    void UploadContext::copyBuffer(vk::Buffer dst, std::span<const std::byte> src,
            vk::DeviceSize dstOffset){
        DE_ASSERT(recording_);
        DE_ASSERT(!src.empty());

        const vk::DeviceSize srcOffset = allocate(src.size());
        staging_->copyToBuffer(src.data(), src.size(), srcOffset);

        cmd_.copyBuffer(*staging_->getBuffer(), dst,
            vk::BufferCopy{}
            .setSrcOffset(srcOffset)
            .setDstOffset(dstOffset)
            .setSize(src.size()));       

        writtenBuffers_.push_back(dst);
    }

    void UploadContext::copyBufferToImage(vk::Image dst, std::span<const std::byte> src,
            vk::Extent3D extent, uint32_t mipLevel, uint32_t layer){
        DE_ASSERT(recording_);
        DE_ASSERT(!src.empty());
        DE_ASSERT(extent.width > 0 && extent.height > 0);

        vk::DeviceSize offset = allocate(src.size());
        staging_->copyToBuffer(src.data(),src.size(), offset);

        cmd_.copyBufferToImage(
            *staging_->getBuffer(),
            dst,
            vk::ImageLayout::eTransferDstOptimal,
            vk::BufferImageCopy{}
                .setBufferOffset(offset)
                .setImageSubresource({vk::ImageAspectFlagBits::eColor, mipLevel, layer, 1})
                .setImageExtent(extent)
        );
    }

    void UploadContext::transitionImageLayout(vk::Image image, vk::ImageLayout oldLayout,
            vk::ImageLayout newLayout, uint32_t layerCount, uint32_t levelCount){

        vk::PipelineStageFlags sourceStage{}, destenationStage{};

		const auto imageSubresource = vk::ImageSubresourceRange{}
			.setAspectMask(vk::ImageAspectFlagBits::eColor)
			.setBaseArrayLayer(0)
			.setLayerCount(layerCount)
			.setLevelCount(levelCount)
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
		 }
		 else if (oldLayout == vk::ImageLayout::eUndefined && newLayout == vk::ImageLayout::eGeneral) {
			 imageBarier.srcAccessMask = {};
			 imageBarier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;

			 sourceStage = vk::PipelineStageFlagBits::eTopOfPipe;
			 destenationStage = vk::PipelineStageFlagBits::eFragmentShader;
		 }else {
			 throw std::runtime_error("unsuported image layout transition");
		 }


		cmd_.pipelineBarrier(
			sourceStage,
			destenationStage,
			vk::DependencyFlags{},
			nullptr,
			nullptr,
			imageBarier);
        
    }

    vk::CommandBuffer UploadContext::cmd() const{
        DE_ASSERT(recording_);            
        return cmd_;
    }

    void UploadContext::retire(std::unique_ptr<Buffer> buffer){
        if(!recording_){
            return;                         
        }
        retired_.push_back(std::move(buffer));
    }

    vk::DeviceSize UploadContext::allocate(vk::DeviceSize size){
        const vk::DeviceSize aligned = alignUp(size, kStagingAlignment);

        if(stagingUsed_ + aligned > stagingCapacity_){
            
            const bool wasRecording = recording_;
            flush();
            if(aligned > stagingCapacity_){
                growStaging(aligned);
            }
            if(wasRecording){
                begin();                    
            }
        }

        const vk::DeviceSize offset = stagingUsed_;
        stagingUsed_ += aligned;
        return offset;
    }

    void UploadContext::growStaging(vk::DeviceSize minSize){
        vk::DeviceSize newCapacity = stagingCapacity_ * 2;
        while(newCapacity < minSize){
            newCapacity *= 2;
        }
        stagingCapacity_ = alignUp(newCapacity, kStagingAlignment);

        staging_ = std::make_unique<Buffer>(device_, MemoryUsageInfo{
            stagingCapacity_, 1, 0, vk::BufferUsageFlagBits::eTransferSrc});
        staging_->map();
        stagingUsed_ = 0;
    }
}