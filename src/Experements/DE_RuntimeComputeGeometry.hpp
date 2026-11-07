#include<memory>
#include<cstdint>
#include<deque>
#include<utility>
#include<vector>

#include <vulkan/vulkan.hpp>

#include"Mesh/DE_Mesh.hpp"
#include"DE_IComputeStep.hpp"
#include"DE_GeometryHandle.hpp"


namespace de{

    class Device;
    class GeometryPool;
    struct GeometryHandle;
    class Shader;
    class DescriptorSetLayout;
    class DescriptorPool;
    class Buffer;
    struct ComputeTag;
    template <typename Tag> class Pipeline;
    using ComputePipeline = Pipeline<ComputeTag>;


    class RuntimeComputeGeometry  final : public IComputeStep{
    public:
        RuntimeComputeGeometry(const Device& device,
             GeometryPool& pool, const GeometryHandle dataPosition,const Mesh::GridDesc grid);
        void updateTime(float deltaTime){ubo.time = deltaTime;}
        
        void drawControlUI();
        void dispatch(vk::CommandBuffer commandBuffer, uint32_t frameIndex) override;
    private:
    
        const Device& device_;
        GeometryPool* pool_;
        const GeometryHandle dataPosition_;

        std::unique_ptr<Shader> computeShader_;
        std::unique_ptr<DescriptorPool> descrptorPool_;
        std::unique_ptr<DescriptorSetLayout> layout_;
        std::unique_ptr<ComputePipeline> computePipeline_;
        vk::DescriptorSet descriptor_;
        std::unique_ptr<Buffer> uboBuffer_;
    
        struct MeshBuffer{
            float time;
            uint32_t numU = 1024;
            uint32_t numV = 1024;
            float minU, maxU;
            float minV, maxV;
            uint32_t p1, p2;
            uint32_t q1, q2;
            float morph;
            float _padding[4];
        }ubo;

        std::deque<std::pair<uint32_t,uint32_t>> morphQueue{{5,8},{5,8}};
        float morphCoef = 0.f;
        float animationSpeed = 1.f;

    };
    
}