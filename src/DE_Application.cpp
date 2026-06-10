#define GLM_FORCE_DEFAUT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_FORCE_RADIANS

//std
#include<iostream>
#include<algorithm>
#include<memory>
#include<utility>
#include<vector>
#include<limits>
#include<glm/glm.hpp>
#include<glm/ext.hpp>

//de
#include"DE_Application.hpp"
#include"DE_Memory.hpp"
#include"Utility/DE_Debug.hpp"
#include"DE_Shaders.hpp"
#include"DE_Pipeline.hpp"
#include"DE_Descriptors.hpp"
#include"DE_CommandBuffers.hpp"
#include"DE_Window.hpp"
#include"DE_RenderPass.hpp"
#include"DE_Renderer.hpp"
#include"Utility/DE_Utility.hpp"



	static std::vector<const char*> getGlfwRequiredExtensions() {
		std::vector<const char*> result;

		uint32_t extCount;
		const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);

		for (uint32_t it{}; it < extCount; ++it) {
			result.emplace_back(glfwExtensions[it]);
		}

		return result;
	}
		
	Application::Application() :
	window_(800,600,"MyEngine") {

		std::vector<const char*> extensions = getGlfwRequiredExtensions();
		std::vector<const char*> layers{};
		std::vector<const char*> deviceExtensions{ VK_KHR_SWAPCHAIN_EXTENSION_NAME };

		instance_ = std::make_unique<de::Instance>("Dimasik",1,"none",1, extensions, layers);
		window_.createSurface(*instance_);
		device_ = std::make_unique<de::Device>(*instance_, *window_.getSurface(), deviceExtensions);
		window_.requestWindowFormat(*device_);

	}

	static void waitIdle(const de::Device& device) {
		device.getLogicalDevice().waitIdle();
	}

	static vk::UniqueSemaphore createSemaphore(const de::Device& device) {
		const auto& logicalDevice = device.getLogicalDevice();

		const auto semaphoreInfo = vk::SemaphoreCreateInfo{};


		return logicalDevice.createSemaphoreUnique(semaphoreInfo);
	}

	static uint32_t aquireNextImageIndex(const de::Device& device, const de::Swapchain& swapchain,uint64_t timeout, vk::Semaphore& semaphore) {
		const auto& logicalDevice = device.getLogicalDevice();

		return logicalDevice.acquireNextImageKHR(*swapchain.getSwapchain(), timeout, semaphore).value;
	}


	static de::GraphicsPipeline createPipeline(const de::Device& device, 
		const de::Window& window, 
		const de::RenderPass& renderPass, 
		const de::Shader& fragmentShader, 
		const de::Shader& vertexShader,
		const std::vector<vk::Format>& vertexFormats) {

		const auto multisampleInfo = vk::PipelineMultisampleStateCreateInfo{};

		std::vector<vk::VertexInputAttributeDescription> vertexAttributes{};
		uint32_t offset{ 0 };
		for (uint32_t it{ 0 }; it < vertexFormats.size(); ++it) {
			vertexAttributes.push_back(vk::VertexInputAttributeDescription{}
				.setBinding(0)
				.setLocation(it)
				.setFormat(vertexFormats[it])
				.setOffset(offset)
			);
			offset += de::utl::getVertexFormatSize(vertexFormats[it]);
		}
		

		const auto bindingDescription = vk::VertexInputBindingDescription{}
			.setBinding(0)
			.setInputRate(vk::VertexInputRate::eVertex)
			.setStride(offset);
		

		const auto vertexInputState = vk::PipelineVertexInputStateCreateInfo{}
			.setVertexAttributeDescriptions(vertexAttributes)
			.setVertexBindingDescriptions(bindingDescription);

		const auto rasterizationState = vk::PipelineRasterizationStateCreateInfo{}
			.setDepthClampEnable(false)
			.setRasterizerDiscardEnable(false)
			.setPolygonMode(vk::PolygonMode::eFill)
			.setLineWidth(1.f);

		const auto colorBlendAttachment = vk::PipelineColorBlendAttachmentState{}
			.setBlendEnable(false)
			.setColorWriteMask(
				vk::ColorComponentFlagBits::eR |
				vk::ColorComponentFlagBits::eG |
				vk::ColorComponentFlagBits::eB |
				vk::ColorComponentFlagBits::eA);
		const auto colorBlendState = vk::PipelineColorBlendStateCreateInfo{}
		.setAttachments(colorBlendAttachment);

		const auto depthStencil = vk::PipelineDepthStencilStateCreateInfo{}
			.setDepthTestEnable(true)
			.setDepthWriteEnable(true)
			.setDepthCompareOp(vk::CompareOp::eLess)
			.setDepthBoundsTestEnable(false)
			.setStencilTestEnable(true);

		const auto builder = de::GraphicsBuilder()
			.addFragmentShader("main", fragmentShader.getShaderModule())
			.addVertexShader("main", vertexShader.getShaderModule())
			.setRenderPass(*renderPass.getRenderPass())
			.setInputAssemblyState(vk::PrimitiveTopology::eTriangleList)
			.setViewportState(window.getExtent())
			.setRasterizationState(rasterizationState)
			.setMultisampleState(multisampleInfo)
			.setVertexInputState(vertexInputState)
			.setColorBlendState(colorBlendState)
			.setDepthStencilState(depthStencil)
			.setVertexInputState(vertexInputState);

		return de::GraphicsPipeline(device, builder);
	}


	static void recordCommandBuffer(const de::RenderPass& renderPass,
		const vk::UniqueFramebuffer& frameBuffer,
		const vk::Extent2D& renderExtent,
		const vk::CommandBuffer& commandBuffer,
		const de::GraphicsPipeline& graphicsPipeline,
		const de::Buffer& vertexBuffer,
		const uint32_t vertexCount) {

		const auto clearValue = std::array<vk::ClearValue, 2>{
			vk::ClearValue{}.setColor(std::array<float,4>{{0.f,0.f,0.5f,1.f}}),
			vk::ClearValue{}.setDepthStencil(vk::ClearDepthStencilValue{1.f,0})
		};

		const auto renderPassBeginInfo = vk::RenderPassBeginInfo{}
			.setRenderPass(*renderPass.getRenderPass())
			.setFramebuffer(*frameBuffer)
			.setRenderArea(vk::Rect2D{ vk::Offset2D{0,0},renderExtent })
			.setClearValues(clearValue);

		const auto beginInfo = vk::CommandBufferBeginInfo{};

		commandBuffer.begin(beginInfo);
		commandBuffer.bindPipeline(vk::PipelineBindPoint::eGraphics, *graphicsPipeline.getPipeline());
		commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
		commandBuffer.bindVertexBuffers(0, *vertexBuffer.getBuffer(), {0} );
		commandBuffer.draw(vertexCount, 1, 0, 0);
		commandBuffer.endRenderPass();
		commandBuffer.end();

	}

	void Application::run() {

		constexpr size_t vertexCount = 36;
		const std::array< glm::vec4, 2 * vertexCount > vertices = {
			// front                            (red)
			glm::vec4{ -.5f, -.5f, .5f, 1.f },  glm::vec4{ 1.f, 0.f, 0.f, 1.f },
			glm::vec4{ .5f, -.5f, .5f, 1.f },   glm::vec4{ 1.f, 0.f, 0.f, 1.f },
			glm::vec4{ -.5f, .5f, .5f, 1.f },   glm::vec4{ 1.f, 0.f, 0.f, 1.f },
			glm::vec4{ .5f, -.5f, .5f, 1.f },   glm::vec4{ 1.f, 0.f, 0.f, 1.f },
			glm::vec4{ .5f, .5f, .5f, 1.f },    glm::vec4{ 1.f, 0.f, 0.f, 1.f },
			glm::vec4{ -.5f, .5f, .5f, 1.f },   glm::vec4{ 1.f, 0.f, 0.f, 1.f },

			// back                             (yellow)
			glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, -.5f, -.5f, 1.f },  glm::vec4{ 1.f, 1.f, 0.f, 1.f },
			glm::vec4{ -.5f, .5f, -.5f, 1.f },  glm::vec4{ 1.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, -.5f, -.5f, 1.f },  glm::vec4{ 1.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, .5f, -.5f, 1.f },   glm::vec4{ 1.f, 1.f, 0.f, 1.f },
			glm::vec4{ -.5f, .5f, -.5f, 1.f },  glm::vec4{ 1.f, 1.f, 0.f, 1.f },

			// left                             (violet)
			glm::vec4{ -.5f, -.5f, .5f, 1.f },  glm::vec4{ 1.f, 0.f, 1.f, 1.f },
			glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 1.f, 0.f, 1.f, 1.f },
			glm::vec4{ -.5f, .5f, -.5f, 1.f },  glm::vec4{ 1.f, 0.f, 1.f, 1.f },
			glm::vec4{ -.5f, -.5f, .5f, 1.f },  glm::vec4{ 1.f, 0.f, 1.f, 1.f },
			glm::vec4{ -.5f, .5f, -.5f, 1.f },  glm::vec4{ 1.f, 0.f, 1.f, 1.f },
			glm::vec4{ -.5f, .5f, .5f, 1.f },   glm::vec4{ 1.f, 0.f, 1.f, 1.f },

			// right                            (green)
			glm::vec4{ .5f, -.5f, .5f, 1.f },   glm::vec4{ 0.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, -.5f, -.5f, 1.f },  glm::vec4{ 0.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, .5f, -.5f, 1.f },   glm::vec4{ 0.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, -.5f, .5f, 1.f },   glm::vec4{ 0.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, .5f, -.5f, 1.f },   glm::vec4{ 0.f, 1.f, 0.f, 1.f },
			glm::vec4{ .5f, .5f, .5f, 1.f },    glm::vec4{ 0.f, 1.f, 0.f, 1.f },

			// top                              (turquoise)
			glm::vec4{ -.5f, -.5f, .5f, 1.f },  glm::vec4{ 0.f, 1.f, 1.f, 1.f },
			glm::vec4{ .5f, -.5f, .5f, 1.f },   glm::vec4{ 0.f, 1.f, 1.f, 1.f },
			glm::vec4{ .5f, -.5f, -.5f, 1.f },  glm::vec4{ 0.f, 1.f, 1.f, 1.f },
			glm::vec4{ -.5f, -.5f, .5f, 1.f },  glm::vec4{ 0.f, 1.f, 1.f, 1.f },
			glm::vec4{ .5f, -.5f, -.5f, 1.f },  glm::vec4{ 0.f, 1.f, 1.f, 1.f },
			glm::vec4{ -.5f, -.5f, -.5f, 1.f }, glm::vec4{ 0.f, 1.f, 1.f, 1.f },

			// bottom                           (blue)
			glm::vec4{ -.5f, .5f, .5f, 1.f },   glm::vec4{ 0.f, 0.f, 1.f, 1.f },
			glm::vec4{ .5f, .5f, .5f, 1.f },    glm::vec4{ 0.f, 0.f, 1.f, 1.f },
			glm::vec4{ .5f, .5f, -.5f, 1.f },   glm::vec4{ 0.f, 0.f, 1.f, 1.f },
			glm::vec4{ -.5f, .5f, .5f, 1.f },   glm::vec4{ 0.f, 0.f, 1.f, 1.f },
			glm::vec4{ .5f, .5f, -.5f, 1.f },   glm::vec4{ 0.f, 0.f, 1.f, 1.f },
			glm::vec4{ -.5f, .5f, -.5f, 1.f },  glm::vec4{ 0.f, 0.f, 1.f, 1.f },
		};

		const auto vertexFormats = std::vector< vk::Format  >{
			vk::Format::eR32G32B32A32Sfloat,
			vk::Format::eR32G32B32A32Sfloat,
		};

		de::MemoryUsageInfo vertexBufferInfo{
			sizeof(vertices),
			0,
			vk::BufferUsageFlagBits::eVertexBuffer
		};
		de::Buffer vertexBuffer(*device_, vertexBufferInfo);		

		const auto& graphicsQueue = device_->getGraphicsQueue();
		const auto& presentQueue = device_->getPresentQueue();

		constexpr uint32_t requestedSwapchainImageCount = 2u;

		de::Shader vertexShader(*device_, "Shaders/bin/vertex.spv");
		de::Shader fragmentShader(*device_, "Shaders/bin/fragment.spv");
		
		const auto& colorFormats = window_.getSurfaceFormats();

		 auto RenderPass = de::RenderPass::Builder{ *device_ }
			.setColorAttachment(colorFormats[0].format)
			.setDepthAttachment(vk::Format::eD32Sfloat)
			.build();

		
		 auto graphicsPipeline = createPipeline(*device_, window_, RenderPass,
			 fragmentShader, vertexShader,vertexFormats);
		

		 std::unique_ptr<de::Swapchain> Swapchain = std::make_unique<de::Swapchain>(de::Swapchain::Builder{ *device_, RenderPass }
			 .setSurface(*window_.getSurface())
			 .setImageFormatAndColorSpace(colorFormats[0])
			 .setImageExtent(window_.getExtent())
			 .setMinImageCount(requestedSwapchainImageCount)
			 .build());


		 de::CommandPool commandPool(*device_, graphicsQueue.queuFamilyIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
		 auto commandBuffer = commandPool.allocate(vk::CommandBufferLevel::ePrimary, requestedSwapchainImageCount);

		 glm::mat4 model{ 1 };
		 glm::mat4 view{ 1 };
		 glm::mat4 projection{ 1 };
		 float rotationAngle = 0.f;

		 auto verticesTemp = vertices;
		while (!window_.shouldClose()) {
			glfwPollEvents();

			if (window_.isResized()) {

				if (window_.isMinimized()) {
					continue;
				}


				const auto& logicalDevice = device_->getLogicalDevice();

				logicalDevice.waitIdle();

				graphicsPipeline = createPipeline(*device_, window_, RenderPass,
					fragmentShader, vertexShader,vertexFormats);

				Swapchain.reset();

				Swapchain = std::make_unique<de::Swapchain>(de::Swapchain::Builder{ *device_, RenderPass }
					.setSurface(*window_.getSurface())
					.setImageFormatAndColorSpace(colorFormats[0])
					.setImageExtent(window_.getExtent())
					.setMinImageCount(requestedSwapchainImageCount)
					.build());

				window_.endResize();
			}
			
			view = glm::translate(glm::mat4{ 1 }, glm::vec3{ 0.f,0.f,-3.f });

			projection = glm::perspective(
				glm::radians(30.0f),
				window_.getExtent().width / (float)window_.getExtent().height,
				0.1f,
				10.0f);

			model = glm::rotate(glm::mat4{ 1 }, rotationAngle, glm::vec3{ 0.f,1.f,0.f });
			for (size_t i = 0; i < vertexCount; ++i) {
				verticesTemp[2 * i] = projection * view * model * vertices[2 * i];
			}
			vertexBuffer.copyToBuffer(verticesTemp);
			rotationAngle += 0.01f;

			const auto frame = Swapchain->getNextFrame();

			recordCommandBuffer(
				RenderPass,
				frame.framebuffer,
				window_.getExtent(),
				commandBuffer.handle(frame.inFlightIndex),
				graphicsPipeline,
				vertexBuffer,
				vertexCount);

			const vk::PipelineStageFlags waitStages[] = {
				vk::PipelineStageFlagBits::eColorAttachmentOutput };

			const auto submitInfo = vk::SubmitInfo{}
				.setCommandBuffers(commandBuffer.handle(frame.inFlightIndex))
				.setPWaitDstStageMask(waitStages)
				.setWaitSemaphores(*frame.readyForRenderingSemaphore)
				.setSignalSemaphores(*frame.readyForPresentingSemaphore);
				graphicsQueue.queue.submit(submitInfo,*frame.inFlightFence);
				
			const auto presentInfo = vk::PresentInfoKHR{}
					.setSwapchains(*Swapchain->getSwapchain())
					.setImageIndices(frame.swapchainImageIndex)
					.setWaitSemaphores(*frame.readyForPresentingSemaphore);
			const auto result = presentQueue.queue.presentKHR(presentInfo);

			if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) {
				throw std::runtime_error("presentation failed");
			}


		
		}
		waitIdle(*device_);
	}

	 