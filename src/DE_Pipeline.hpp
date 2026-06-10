#ifndef _DE_PIPELINE_
#define _DE_PIPELINE_

#include<memory>
//vk
#include <vulkan/vulkan.hpp>
//de
#include "DE_PipelineTraits.hpp"

namespace de{

	class Device;
	template<typename Tag>
	class Pipeline{
	public:
		using Traits = PipelineTraits<Tag>;
		using Builer = typename Traits::Builder;

		Pipeline(const Device& device, const Builer& builder) {

			//alliases
			const auto& logicalDevice = device.getLogicalDevice();
			const auto& dsLayouts = builder.getDsLayouts();
			const auto& pushRanges = builder.getPushConstantRanges();

			const auto pipelineLayoutInfo = vk::PipelineLayoutCreateInfo{}
				.setSetLayouts(dsLayouts)
				.setPushConstantRanges(pushRanges);

			pipelineLayout_ = logicalDevice.createPipelineLayoutUnique(pipelineLayoutInfo);

			pipeline_ = Traits::create(logicalDevice, *pipelineLayout_, builder);
		}
		~Pipeline() = default;

		Pipeline(const Pipeline&) = delete;
		Pipeline& operator=(const Pipeline&) = delete;

		Pipeline(Pipeline&&) = default;
		Pipeline& operator=(Pipeline&&) = default;

		const vk::UniquePipeline& getPipeline() const noexcept { return pipeline_; }
		const vk::UniquePipelineLayout& getLayout() const noexcept { return pipelineLayout_; }

	private:
		vk::UniquePipeline pipeline_;
		vk::UniquePipelineLayout pipelineLayout_;

	};

	inline auto ComputeBuilder(){
		return de::Pipeline<ComputeTag>::Traits::Builder{};
	}
	inline auto GraphicsBuilder() {
		return  std::move(de::Pipeline<GraphicsTag>::Traits::Builder{});
	}

	using ComputePipeline = Pipeline<ComputeTag>;
	using GraphicsPipeline = Pipeline<GraphicsTag>;
	
}



#endif 

