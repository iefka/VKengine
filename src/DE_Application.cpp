
#include "DE_Application.hpp"
#include "DE_Memory.hpp"
#include "DE_Shaders.hpp"
#include "DE_Pipeline.hpp"
#include "DE_RenderPass.hpp"
#include "DE_Renderer.hpp"
#include "DE_CommandBuffers.hpp"
#include "DE_PushConstants.hpp"
#include "Utility/DE_Debug.hpp"
#include "Utility/DE_Utility.hpp"


#include <array>
#include <stdexcept>


// ============================================================================
// AppTimer Implementation
// ============================================================================

struct GlobalUBO {
	glm::mat4 projectionView{ 1.f };
	glm::vec3 lightDirection = glm::normalize(glm::vec3(1.f, 3.f, 2.f));
};

AppTimer::AppTimer() { reset(); }

void AppTimer::reset() {
	startTime_ = std::chrono::steady_clock::now();
	lastFrameTime_ = startTime_;
	totalTime_ = 0.0f;
	deltaTime_ = 0.0f;
}

float AppTimer::getDeltaTime() {
	auto currentTime = std::chrono::steady_clock::now();
	deltaTime_ = std::chrono::duration<float>(currentTime - lastFrameTime_).count();
	totalTime_ = std::chrono::duration<float>(currentTime - startTime_).count();
	lastFrameTime_ = currentTime;
	return deltaTime_;
}

float AppTimer::getTotalTime() const { return totalTime_; }

// ============================================================================
// Application Implementation
// ============================================================================

Application::Application(const ApplicationConfig& config)
	: config_(config), window_(config.windowWidth, config.windowHeight, config.windowTitle) {
	initVulkan();
	initResources();
}

Application::~Application() {}


void Application::initVulkan() {
	auto extensions = getRequiredExtensions();
	std::vector<const char*> layers{};

	instance_ = std::make_unique<de::Instance>(
		config_.applicationName,
		config_.applicationVersion,
		config_.engineName,
		config_.engineVersion,
		extensions,
		layers
	);

	window_.createSurface(*instance_);

	std::vector<const char*> deviceExtensions{VK_KHR_SWAPCHAIN_EXTENSION_NAME};
	device_ = std::make_unique<de::Device>(*instance_, *window_.getSurface(), deviceExtensions);

	window_.requestWindowFormat(*device_);
	surfaceFormat_ = window_.getSurfaceFormats()[0];
}

void Application::initResources() {
	
	initDescriptorSets();
	initGameObjects();
	initRenderPass();
	initRenderSystem();
	initCommandPool();
	initSwapchain();


}

void Application::initDescriptorSets() {

	globalPool_ = std::make_unique<de::DescriptorPool>(
		de::DescriptorPool::Builder(*device_)
		.setMaxSets(config_.swapchainImageCount)
		.addPoolSize(vk::DescriptorType::eUniformBuffer, config_.swapchainImageCount)
		.build()
	);

	de::MemoryUsageInfo uboBufferInfo{
		sizeof(GlobalUBO),
		1,
		0,
		vk::BufferUsageFlagBits::eUniformBuffer,
		vk::MemoryPropertyFlagBits::eHostVisible
	};
	for (size_t i = 0; i < config_.swapchainImageCount; ++i){
		uboBuffers_.push_back(
			std::make_unique<de::Buffer>(*device_, uboBufferInfo)
		);
		uboBuffers_[i]->map();
	}
	
	descriptorLayouts_.push_back(de::DescriptorSetLayout::Builder{ *device_ }
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eVertex)
		.build()
	);

}

void Application::initGameObjects() {

	controlledCamera_ = std::make_unique<de::ControlledCamera>(window_);
	std::unique_ptr<de::Model> smoothVaseModel = de::Model::createModelFromFile(*device_,"../Models/smooth_vase.obj");
	auto smoothVase = de::GameObject::createGameObject();
	smoothVase.model = std::move(smoothVaseModel);
	smoothVase.transform.rotation.x = glm::radians(180.f);
	smoothVase.transform.translation = { .0f, .0f, 0.f };
	smoothVase.transform.scale = glm::vec3(3.f);
	gameObjects.push_back(std::move(smoothVase));

	std::unique_ptr<de::Model> cubeModel = de::Model::createModelFromFile(*device_, "../Models/cube.obj");
	auto cube = de::GameObject::createGameObject();
	cube.model = std::move(cubeModel);
	cube.transform.rotation.x = glm::radians(180.f);
	cube.transform.scale = { 0.2f,0.2f,0.2f };
	cube.transform.translation = { 1.f, .0f, 0.f };
	gameObjects.push_back(std::move(cube));

}

void Application::initRenderSystem(){
	renderSystem_ = std::make_unique<de::RenderSystem>(
		de::RenderSystem(*device_, *renderPass_, window_.getExtent(),descriptorLayouts_)
	);
}

void Application::initRenderPass() {
	renderPass_ = std::make_unique<de::RenderPass>(
		de::RenderPass::Builder{*device_}
		.setColorAttachment(surfaceFormat_.format)
		.setDepthAttachment(vk::Format::eD32Sfloat)
		.build()
	);
}


void Application::initSwapchain() {
	swapchain_ = std::make_unique<de::Swapchain>(
		de::Swapchain::Builder{*device_, *renderPass_}
		.setSurface(*window_.getSurface())
		.setImageFormatAndColorSpace(surfaceFormat_)
		.setImageExtent(window_.getExtent())
		.setMinImageCount(config_.swapchainImageCount)
		.build()
	);
	swapchain_->createFrameResources(*commandPool_);
	swapchain_->createFrameDescriptorSets(*globalPool_, descriptorLayouts_[0], uboBuffers_);
}

void Application::initCommandPool () {
	const auto& graphicsQueue = device_->getGraphicsQueue();
	commandPool_ = std::make_unique<de::CommandPool>(*device_, graphicsQueue.queuFamilyIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
}

std::vector<const char*> Application::getRequiredExtensions() {
	std::vector<const char*> extensions;
	uint32_t extCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);
	for (uint32_t i = 0; i < extCount; ++i) extensions.push_back(glfwExtensions[i]);
	return extensions;
}

std::pair<vk::Viewport, vk::Rect2D> getViewportState(const vk::Extent2D& viewportExtent) {

	auto viewport = vk::Viewport{}
		.setX(0.f)
		.setY(0.f)
		.setWidth(static_cast<float>(viewportExtent.width))
		.setHeight(static_cast<float>(viewportExtent.height))
		.setMinDepth(0.f)
		.setMaxDepth(1.f);

	auto scissors = vk::Rect2D{ {0,0},viewportExtent };

	return std::pair<vk::Viewport, vk::Rect2D>(viewport, scissors);
};

void Application::recordCommandBuffer(const de::FrameData& frameData) {
	auto clearValues = std::array<vk::ClearValue, 2>{
		vk::ClearValue{}.setColor(std::array<float, 4>{{0.0f, 0.0f, 0.0f, 1.0f}}), 
		vk::ClearValue{}.setDepthStencil(vk::ClearDepthStencilValue{1.0f, 0})};

	auto renderPassBeginInfo = vk::RenderPassBeginInfo{}
		.setRenderPass(*renderPass_->getRenderPass())
		.setFramebuffer(*frameData.framebuffer)
		.setRenderArea(vk::Rect2D{vk::Offset2D{0, 0}, window_.getExtent()})
		.setClearValues(clearValues);

	auto beginInfo = vk::CommandBufferBeginInfo{};

	frameData.commandBuffer.begin(beginInfo);
	frameData.commandBuffer.beginRenderPass(renderPassBeginInfo, vk::SubpassContents::eInline);
	auto [viewport, scissors] = getViewportState(window_.getExtent());

	frameData.commandBuffer.setViewport(0, viewport);
	frameData.commandBuffer.setScissor(0, scissors);
	renderSystem_->renderGameObjects(frameData.commandBuffer, gameObjects, controlledCamera_->getCamera(),frameData.descriptor);

	frameData.commandBuffer.endRenderPass();
	frameData.commandBuffer.end();
}

void Application::handleWindowResize() {
	if (!window_.isResized()) return;
	if (window_.isMinimized()) return;
	device_->getLogicalDevice().waitIdle();
	recreateSwapchain();
	window_.endResize();
}

void Application::recreateSwapchain() { swapchain_.reset(); initSwapchain(); }

void Application::renderFrame() {
	auto frame = swapchain_->getNextFrame();

	const auto& camera = controlledCamera_->getCamera();
	const auto extent = window_.getExtent();
	auto aspectRaito = static_cast<float>(extent.width) / static_cast<float>(extent.height);
	GlobalUBO ubo{};
	ubo.projectionView = camera.getProjectionMatrix(aspectRaito) * camera.getViewMatrix();


	uboBuffers_[frame.inFlightIndex]->copyToBuffer(&ubo,sizeof(GlobalUBO),0);
	uboBuffers_[frame.inFlightIndex]->flush();

	recordCommandBuffer(frame);

	const vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eColorAttachmentOutput };

	auto submitInfo = vk::SubmitInfo{}.setCommandBuffers(frame.commandBuffer)
		.setPWaitDstStageMask(waitStages)
		.setWaitSemaphores(*frame.readyForRenderingSemaphore)
		.setSignalSemaphores(*frame.readyForPresentingSemaphore);

	device_->getGraphicsQueue().queue.submit(submitInfo, *frame.inFlightFence);

	auto presentInfo = vk::PresentInfoKHR{}
		.setSwapchains(*swapchain_->getSwapchain())
		.setImageIndices(frame.swapchainImageIndex)
		.setWaitSemaphores(*frame.readyForPresentingSemaphore);

	auto result = device_->getPresentQueue().queue.presentKHR(presentInfo);
	if (result != vk::Result::eSuccess && result != vk::Result::eSuboptimalKHR) throw std::runtime_error("Presentation failed");
}

void Application::mainLoop() {
	while (!window_.shouldClose()) {
		glfwPollEvents();
		handleWindowResize();
		float deltaTime = timer_.getDeltaTime();
		handleInputs(deltaTime);
		updateScene(deltaTime);
		renderFrame();
	}
	cleanup();
}

void Application::cleanup() { if (device_) device_->getLogicalDevice().waitIdle(); }

void Application::run() { 
	timer_.reset(); 
	mainLoop(); 
}

void Application::handleInputs(float deltaTime) {
	
	controlledCamera_->handleInputs(deltaTime, GLFW_KEY_C);
}

void Application::updateScene(float deltaTime) {
	float speed = 1.0f; // радиан в секунду
	gameObjects[0].transform.rotation.y += speed * deltaTime;
	controlledCamera_->updateCamera(deltaTime);
}