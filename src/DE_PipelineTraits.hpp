#pragma once

#include<vulkan/vulkan.hpp>

#include<span>
#include<array>

namespace de{

    struct ComputeTag{};
    struct GraphicsTag{};

    // basic template
    template<typename Tag>
    struct PipelineTraits;

    // compute pipeline specialization
    template<>
    struct PipelineTraits<ComputeTag>{
        struct Builder{
           
            
            
            Builder& setShaderStage(const vk::PipelineShaderStageCreateInfo& stage) noexcept{
                shaderStage_ = stage;
                return *this;
            }

            Builder& addDsLayout(vk::DescriptorSetLayout layout) noexcept {
                dsLayouts_.push_back(layout);
                return *this;
            }

            Builder& setPushConstantRanges(const std::vector<vk::PushConstantRange>& ranges) noexcept {
                pushConstantRanges_ = ranges;
                return *this;
            }
            Builder& setCache(vk::PipelineCache cache)noexcept{
                cache_ = cache;
                return *this;
            }

            // geters

            const std::vector<vk::DescriptorSetLayout>& getDsLayouts() const noexcept { return dsLayouts_; }
            const std::vector<vk::PushConstantRange>& getPushConstantRanges() const noexcept { return pushConstantRanges_; }
            const vk::PipelineShaderStageCreateInfo& getShaderStage() const noexcept{ return shaderStage_; }
            const vk::PipelineCache getCache() const noexcept {return cache_;}

        private:
            std::vector<vk::DescriptorSetLayout> dsLayouts_;
            std::vector<vk::PushConstantRange> pushConstantRanges_;
            vk::PipelineShaderStageCreateInfo shaderStage_;
            vk::PipelineCache cache_{};
        };

        static vk::UniquePipeline create(const vk::Device& device,
            const vk::PipelineLayout& layout,
            const Builder& builder){
            const auto computePipelineInfo = vk::ComputePipelineCreateInfo{}
                .setLayout(layout)
                .setStage(builder.getShaderStage());

            auto [result, pipeline] = device.createComputePipelineUnique(builder.getCache(), computePipelineInfo);
            if (result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to create compute pipeline");
            }
            
            return std::move(pipeline);
        }
    };


    //graphic pipeline
    template<>
    struct PipelineTraits<GraphicsTag> {

        struct Builder {

            //add fragment shader stage to vector
            Builder& addVertexShader(const char* entryPointName, vk::ShaderModule shaderModule) {

                stageInfos_.push_back(
                    vk::PipelineShaderStageCreateInfo{}
                    .setStage(vk::ShaderStageFlagBits::eVertex)
                    .setPName(entryPointName)
                    .setModule(shaderModule)
                );
                return *this;
            }
            //add fragment shader stage to vector
            Builder& addFragmentShader(const char* entryPointName, vk::ShaderModule shaderModule) {

                stageInfos_.push_back(  
                    vk::PipelineShaderStageCreateInfo{}
                    .setStage(vk::ShaderStageFlagBits::eFragment)
                    .setPName(entryPointName)
                    .setModule(shaderModule)  
                );
                return *this;
            }
            //add any shader stage to vector
            Builder& addShaderStage(vk::PipelineShaderStageCreateInfo stage) {
                stageInfos_.push_back(stage);
                return *this;
            }

            //set vertex input state
            /*Builder& setVertexInputState(const std::vector<const vk::VertexInputAttributeDescription>& atributeDescriptions,
                const std::vector<const vk::VertexInputBindingDescription>& bindingDescriptions) {
                inputStateCreateInfo_ = vk::PipelineVertexInputStateCreateInfo{}
                    .setVertexAttributeDescriptions(atributeDescriptions)
                    .setVertexBindingDescriptions(bindingDescriptions);
                
                return *this;
            }*/

            Builder& setVertexInputState(vk::PipelineVertexInputStateCreateInfo inputStateCreateInfo) {
                inputStateCreateInfo_ = inputStateCreateInfo;

                return *this;
            }
            //set assembly input state
            Builder& setInputAssemblyState(vk::PrimitiveTopology topology) {
                inputAssemblyStateCreateInfo_ = vk::PipelineInputAssemblyStateCreateInfo{}
                .setTopology(topology);
                return *this;
            }
            Builder& setViewportState(const vk::Extent2D& viewportExtent) {
                
                viewport_ = vk::Viewport{}
                    .setX(0.f)
                    .setY(0.f)
                    .setWidth(static_cast<float>(viewportExtent.width))
                    .setHeight(static_cast<float>(viewportExtent.height))
                    .setMinDepth(0.f)
                    .setMaxDepth(1.f);

                scissors_ = vk::Rect2D{ {0,0},viewportExtent };

                viewportInfo_ = vk::PipelineViewportStateCreateInfo{}
                    .setViewports(viewport_)
                    .setScissors(scissors_);

                return *this;
            }

            Builder& setRasterizationState(const vk::PipelineRasterizationStateCreateInfo rasterezationInfo) {
                rasterezationInfo_ = rasterezationInfo;
                return *this;
            }
            Builder& setMultisampleState(const vk::PipelineMultisampleStateCreateInfo multisampleInfo) {
                multisampleInfo_ = multisampleInfo;
                return *this;
            }
            Builder& setColorBlendState(const vk::PipelineColorBlendStateCreateInfo colorBlendState) {
                colorBlendState_ = colorBlendState;
                return *this;
            }
            [[deprecated]] Builder& setRenderPass(const vk::RenderPass renderPass){
                renderPass_ = renderPass;
                return  *this;
            }

            //fully replace all data
            Builder& setDsLayouts(const std::span<const vk::DescriptorSetLayout> layouts) noexcept {
                dsLayouts_.assign_range(layouts);
                return *this;
            }
            //add all to the end of the vector
            Builder& addDsLayouts(const std::span<const vk::DescriptorSetLayout> layouts) noexcept {
                dsLayouts_.append_range(layouts);
                return *this;
            }

            Builder& addDsLayout(vk::DescriptorSetLayout layout) noexcept {
                dsLayouts_.push_back(layout);
                return *this;
            }

            Builder& setPushConstantRanges(const std::vector<vk::PushConstantRange>& ranges) noexcept {
                pushConstantRanges_ = ranges;
                return *this;
            }

            Builder& setDepthStencilState(const vk::PipelineDepthStencilStateCreateInfo& depthState) {
                detphState_ = depthState;
                return *this;
            }

            Builder& setDynamicStates(const std::vector<vk::DynamicState>& dynamicStates) {
                dynamicState
                    .setDynamicStates(dynamicStates);
                return *this;
            }

            Builder& addPushConstantRange(vk::PushConstantRange range) noexcept {
                pushConstantRanges_.push_back(range);
                return *this;
            }
            Builder& setPipelineCache(vk::PipelineCache cache) noexcept {
                cache_ = cache;
                return *this;
            }
            Builder& setRenderingFormat(vk::Format color, vk::Format depth){

                colorAttachment_[0] = color;
                renderingInfo_
                .setColorAttachmentFormats(colorAttachment_)
                .setDepthAttachmentFormat(depth);
                return *this;
            }

           const std::vector<vk::PipelineShaderStageCreateInfo>& getStageInfos()const noexcept          {return stageInfos_;}
           const vk::PipelineVertexInputStateCreateInfo*         getInputStateInfo() const noexcept     {return &inputStateCreateInfo_;}
           const vk::PipelineInputAssemblyStateCreateInfo*       getAssemblyStateInfo()const noexcept   {return &inputAssemblyStateCreateInfo_;}
           const vk::PipelineViewportStateCreateInfo*            getViewportInfo()const noexcept        {return &viewportInfo_;}
           const vk::PipelineRasterizationStateCreateInfo*       getRasterezationInfo()const noexcept   {return &rasterezationInfo_;}
           const vk::PipelineMultisampleStateCreateInfo*         getMultisampleInfo()const noexcept     {return &multisampleInfo_;}
           const vk::PipelineColorBlendStateCreateInfo*          getColorBlendInfo()const noexcept      {return &colorBlendState_;}
           [[deprecated]]const vk::RenderPass&                   getRenderPass()const noexcept          {return renderPass_;}
           const std::vector<vk::DescriptorSetLayout>&           getDsLayouts() const noexcept          {return dsLayouts_; }
           const std::vector<vk::PushConstantRange>&             getPushConstantRanges() const noexcept {return pushConstantRanges_; }
           const vk::PipelineDepthStencilStateCreateInfo*        getDepthInfo() const noexcept          {return &detphState_;}
           const vk::PipelineDynamicStateCreateInfo*             getDynamicState() const noexcept       { return &dynamicState; }
           const vk::PipelineCache                               getCache() const noexcept              {return cache_;}
           const vk::PipelineRenderingCreateInfo*               getRenderingInfo() const noexcept       {return &renderingInfo_;}

        private:
            std::vector<vk::PipelineShaderStageCreateInfo> stageInfos_;
            //vertex input
            vk::PipelineVertexInputStateCreateInfo   inputStateCreateInfo_{};
            vk::PipelineInputAssemblyStateCreateInfo inputAssemblyStateCreateInfo_{};
            vk::PipelineViewportStateCreateInfo      viewportInfo_{};
            vk::PipelineRasterizationStateCreateInfo rasterezationInfo_{};
            vk::PipelineMultisampleStateCreateInfo   multisampleInfo_{};
            vk::PipelineColorBlendStateCreateInfo    colorBlendState_{};
            vk::PipelineDepthStencilStateCreateInfo  detphState_{};
            vk::RenderPass                           renderPass_;
            vk::PipelineDynamicStateCreateInfo       dynamicState{};
            vk::PipelineRenderingCreateInfo          renderingInfo_{};

            //need for setRenderingFormat because value(color) die after scoupe
            std::array<vk::Format, 1> colorAttachment_;
            vk::Viewport viewport_{};
            vk::Rect2D scissors_{};
            std::vector<vk::DescriptorSetLayout> dsLayouts_;
            std::vector<vk::PushConstantRange> pushConstantRanges_;
            vk::PipelineCache cache_;

            
        };

        static vk::UniquePipeline create(const vk::Device& device,
            const vk::PipelineLayout& layout,
            const Builder& builder) {
            const auto graphicsPipelineInfo = vk::GraphicsPipelineCreateInfo{}
                .setLayout(layout)
                .setPVertexInputState(builder.getInputStateInfo())
                .setPInputAssemblyState(builder.getAssemblyStateInfo())
                .setStages(builder.getStageInfos())
                .setPViewportState(builder.getViewportInfo())
                .setPRasterizationState(builder.getRasterezationInfo())
                .setPMultisampleState(builder.getMultisampleInfo())
                .setRenderPass(nullptr)
                .setPDynamicState(builder.getDynamicState())
                .setPColorBlendState(builder.getColorBlendInfo())
                .setPDepthStencilState(builder.getDepthInfo())
                .setPNext(builder.getRenderingInfo());

      

            auto [result, pipeline] = device.createGraphicsPipelineUnique(builder.getCache(), graphicsPipelineInfo);
            if (result != vk::Result::eSuccess) {
                throw std::runtime_error("Failed to create graphics pipeline");
            }

            return std::move(pipeline);
        }

    };

}
