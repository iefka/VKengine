#include"DE_Device.hpp"
#include"DE_GeometryPool.hpp"
#include"DE_Shaders.hpp"
#include"DE_Descriptors.hpp"
#include"DE_Pipeline.hpp"
#include"DE_Memory.hpp"
#include"third_party/imgui/imgui.h"
#include"DE_RuntimeComputeGeometry.hpp"

#include <array>


namespace de{
  
    namespace{
        float easing(float x){
	        return (x < 0.5)
		        ? (4 * x * x * (3 * x - 1))
		        : (4 * (x - 1) * (x - 1) * (3 * (x - 1) + 1) + 1);
        }
    }

    RuntimeComputeGeometry::RuntimeComputeGeometry(const Device& device,
        GeometryPool& pool, const GeometryHandle dataPosition, const Mesh::GridDesc grid)
        :device_{device}
        ,pool_{&pool}
        ,dataPosition_{dataPosition}{

        ubo.numU = grid.resolutionU;
        ubo.numV = grid.resolutionV;

        computeShader_ = std::make_unique<Shader>(device_,
            "Shaders/bin/torus_knot.comp.spv");
        
        descrptorPool_ = std::make_unique<DescriptorPool>(
            DescriptorPool::Builder{device_}
                .setMaxSets(2)
                .setPoolFlags(vk::DescriptorPoolCreateFlagBits::eUpdateAfterBind)
                .addPoolSize(vk::DescriptorType::eUniformBuffer,1)
                .addPoolSize(vk::DescriptorType::eStorageBuffer,1)
                .build()
        );

        //each each element consist flag for corresponding binding
        constexpr std::array<vk::DescriptorBindingFlags, 2> flags = {
            vk::DescriptorBindingFlags{}, 
            {vk::DescriptorBindingFlagBits::eUpdateAfterBind}
        };
        const auto bindingInfo  =vk::DescriptorSetLayoutBindingFlagsCreateInfo{}
            .setBindingFlags(flags);

        layout_ = std::make_unique<DescriptorSetLayout>(
            DescriptorSetLayout::Builder{device_}
                .addBinding(0,vk::DescriptorType::eUniformBuffer,
                vk::ShaderStageFlagBits::eCompute)
                .addBinding(1,vk::DescriptorType::eStorageBuffer,
                vk::ShaderStageFlagBits::eCompute)
                .setFlags({vk::DescriptorSetLayoutCreateFlagBits::eUpdateAfterBindPool})
                .setPNext(&bindingInfo)
                .build()
        );

        uboBuffer_ = std::make_unique<Buffer>(
            device_,
            MemoryUsageInfo{
                sizeof(MeshBuffer),
                1,0, 
                vk::BufferUsageFlagBits::eUniformBuffer,
                vk::MemoryPropertyFlagBits::eHostVisible
            }
        );
        uboBuffer_->map();

        auto uboDescriptorInfo = uboBuffer_->getDescriptorInfo();
        auto bufferDescriptorInfo = pool_->getVertexRangeDescriptor(dataPosition_.vertexOffset,
            dataPosition_.vertexCount);
        descriptor_ = DescriptorWriter{device_,*descrptorPool_,*layout_}
            .writeBuffer(0,&uboDescriptorInfo)
            .writeBuffer(1,&bufferDescriptorInfo)
            .build();
        
        
        

        computePipeline_ = std::make_unique<ComputePipeline>( device_,
            ComputePipeline::Builer{}
                .addDsLayout(layout_->getLayout())
                .setShaderStage(computeShader_->getStageInfo(
                    vk::ShaderStageFlagBits::eCompute)
                )
        );
    }

    void RuntimeComputeGeometry::dispatch(vk::CommandBuffer commandBuffer,
        uint32_t frameIndex){

        const auto& pipeline = *computePipeline_->getPipeline();
        const auto& layout = *computePipeline_->getLayout();

        auto iter = morphQueue.begin();
        ubo.morph = easing(morphCoef);
        ubo.p1 = iter->first;
        ubo.q1 = iter->second;
        ubo.p2 = (iter + 1)->first;
        ubo.q2 = (iter + 1)->second;

        ubo.minU = -1.0f;
        ubo.maxU = +1.0f;
        ubo.minV = -1.0f;
        ubo.maxV = +1.0f;


        uboBuffer_->copyToBuffer(&ubo,sizeof(MeshBuffer),0);
        uboBuffer_->flush();

        morphCoef += animationSpeed * ubo.time;
        if (morphCoef > 1.0)
        {
            morphCoef = 1.0f;
            if (morphQueue.size() > 2)
            {
                morphCoef = 0.0f;
                morphQueue.pop_front();
            }
        }
#if 1
         const auto writeReadybarrier = vk::BufferMemoryBarrier{}
            .setSrcAccessMask({})
            .setDstAccessMask({})
            .setBuffer(pool_->getVertexBufferHandle())
            .setOffset(static_cast<vk::DeviceSize>(dataPosition_.vertexOffset) * sizeof(Mesh::Vertex))
            .setSize(static_cast<vk::DeviceSize>(dataPosition_.vertexCount) * sizeof(Mesh::Vertex))
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eVertexInput,
            {},{},
            writeReadybarrier,
            {}
        );
#endif

        
    auto bufferDescriptorInfo = pool_->getVertexRangeDescriptor(dataPosition_.vertexOffset,
            dataPosition_.vertexCount);
            DescriptorWriter{device_,*descrptorPool_,*layout_}
            .writeBuffer(1,&bufferDescriptorInfo)
            .overwrite(descriptor_);


        commandBuffer.bindPipeline(vk::PipelineBindPoint::eCompute, pipeline);
        commandBuffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, layout,
            0, {descriptor_}, {});
        
        const uint32_t groupX = (ubo.numU + 15) / 16;
        const uint32_t groupY = (ubo.numV + 15) / 16;
        commandBuffer.dispatch(groupX, groupY, 1);

        const auto readReadybarrier = vk::BufferMemoryBarrier{}
            .setSrcAccessMask(vk::AccessFlagBits::eShaderWrite)
            .setDstAccessMask(vk::AccessFlagBits::eVertexAttributeRead)
            .setBuffer(pool_->getVertexBufferHandle())
            .setOffset(static_cast<vk::DeviceSize>(dataPosition_.vertexOffset) * sizeof(Mesh::Vertex))
            .setSize(static_cast<vk::DeviceSize>(dataPosition_.vertexCount) * sizeof(Mesh::Vertex))
            .setSrcQueueFamilyIndex(vk::QueueFamilyIgnored)
            .setDstQueueFamilyIndex(vk::QueueFamilyIgnored);

        commandBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eComputeShader,
            vk::PipelineStageFlagBits::eVertexInput,
            {},{},
            readReadybarrier,
            {}
        );
    }


    void RuntimeComputeGeometry::drawControlUI(){

        static const std::vector<std::pair<uint32_t, uint32_t>> PQ = {
	        {2, 3}, {2, 5}, {2, 7}, {3, 4}, {2, 9}, {3, 5}, {5 ,8}, {11, 11}
        };

        if (ImGui::CollapsingHeader("Thorus knot")) {
            
            ImGui::SliderFloat("Animation speed", &animationSpeed, 0.0f, 2.0f);

            for (size_t i = 0; i != PQ.size(); i++)
            {
                std::string title = std::to_string(PQ[i].first) + ", " + std::to_string(PQ[i].second);
                if (ImGui::Button(title.c_str()))
                {
                    if (PQ[i] != morphQueue.back())
                        morphQueue.push_back(PQ[i]);
                }
            }

        }
    }
} 
