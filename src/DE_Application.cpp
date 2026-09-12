
#include "DE_Application.hpp"
#include "DE_Memory.hpp"
#include "DE_Shaders.hpp"
#include "DE_Pipeline.hpp"
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

	window_.selectSurfaceFormat(*device_,{vk::Format::eR8G8B8A8Srgb,vk::ColorSpaceKHR::eSrgbNonlinear});
}

void Application::initResources() {
	
	initDescriptorSets();
	initCommandPool();
	initGameObjects();
	initRenderer();
	initSystems();
	preparePasses();
}

void Application::initDescriptorSets() {


	constexpr uint32_t maxUniqueTextures = 64;

	globalPool_ = std::make_unique<de::DescriptorPool>(
		de::DescriptorPool::Builder(*device_)
		.setMaxSets(maxUniqueTextures)
		.addPoolSize(vk::DescriptorType::eCombinedImageSampler, maxUniqueTextures)
		.build()
	);

	//set 0
	descriptorLayouts_.push_back(de::DescriptorSetLayout::Builder{ *device_ }
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAllGraphics)
		.build()
	);

	//set 1
	descriptorLayouts_.push_back(de::DescriptorSetLayout::Builder{ *device_ }
		.addBinding(0, vk::DescriptorType::eCombinedImageSampler, vk::ShaderStageFlagBits::eFragment)
		.build()
	);

	

}

void Application::preparePasses(){

	mainPass_ = std::make_unique<de::MainRenderPass>(*device_,
		*renderer_,
		*renderSystem_,
		*pointLightSystem_,
		*controlledCamera_,
		gameObjects,
		descriptorLayouts_[0]);

	renderer_->registerPass(mainPass_.get());
}

void Application::initGameObjects() {


	materialManager_ = std::make_unique<de::MaterialManager>(*device_, *globalPool_ ,descriptorLayouts_[1], "Materials");

	controlledCamera_ = std::make_unique<de::ControlledCamera>(window_);

	std::unique_ptr<de::Model> smoothVaseModel = de::Model::createModelFromFile(*device_,"Models/smooth_vase.obj",*materialManager_);
	auto smoothVase = de::GameObject::createGameObject();
	smoothVase.model = std::move(smoothVaseModel);
	smoothVase.transform.rotation.x = glm::radians(180.f);
	smoothVase.transform.translation = { .0f, .0f, 0.f };
	smoothVase.transform.scale = glm::vec3(3.f);
	gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));


	std::unique_ptr<de::Model>flatVaseModel = de::Model::createModelFromFile(*device_, "Models/flat_vase.obj", *materialManager_);
	auto flatVase = de::GameObject::createGameObject();	
	flatVase.model = std::move(flatVaseModel);
	flatVase.transform.rotation.x = glm::radians(180.f);
	flatVase.transform.translation = { .6f, .0f, -0.8f };
	flatVase.transform.scale = glm::vec3(3.f);
	gameObjects.emplace(flatVase.getId(), std::move(flatVase));

	std::unique_ptr<de::Model> cubeModel = de::Model::createModelFromFile(*device_, "Models/cube.obj", *materialManager_);
	auto cube = de::GameObject::createGameObject();
	cube.model = std::move(cubeModel);
	cube.transform.rotation.x = glm::radians(180.f);
	cube.transform.scale = { 0.2f,0.2f,0.2f };
	cube.transform.translation = { 1.f, 0.2f, 0.f };
	gameObjects.emplace(cube.getId(), std::move(cube));

	std::unique_ptr<de::Model> quadModel = de::Model::createModelFromFile(*device_, "Models/quad.obj", *materialManager_);
	auto flor = de::GameObject::createGameObject();
	flor.model = std::move(quadModel);
	flor.transform.rotation.x = glm::radians(180.f);
	flor.transform.scale = { 3.f, 1.f, 3.f };
	flor.transform.translation = {0.f, 0.0f, 0.f};
	gameObjects.emplace (flor.getId(), std::move(flor));


	std::vector<glm::vec3> lightColors{
	  {1.f, .1f, .1f},
	  {.1f, .1f, 1.f},
	  {.1f, 1.f, .1f},
	  {1.f, 1.f, .1f},
	  {.1f, 1.f, 1.f},
	  {1.f, 1.f, 1.f}  //
	};

	for (int i = 0; i < lightColors.size(); i++) {
		auto pointLight = de::GameObject::makePointLight(0.2f,0.1f);
		pointLight.color = lightColors[i];
		auto rotateLight = glm::rotate(
			glm::mat4(1.f),
			(i * glm::two_pi<float>()) / lightColors.size(),
			{ 0.f, 1.f, 0.f });
		pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(1.f, 1.f, 1.f, 1.f));
		gameObjects.emplace(pointLight.getId(), std::move(pointLight));
	}
}

void Application::initSystems(){
	renderSystem_ = std::make_unique<de::RenderSystem>(
		*device_, renderer_->getMainRenderPass(), window_.getExtent(), descriptorLayouts_
	);
	pointLightSystem_ = std::make_unique<de::PointLightSystem>(
		*device_, renderer_->getMainRenderPass(), window_.getExtent(), descriptorLayouts_
	);
}

//void Application::initRenderPass() {
	
//}


void Application::initRenderer() {
	renderer_ = 
		std::make_unique<de::Renderer>(
			*device_,
			window_,
			*commandPool_,
			config_.swapchainImageCount,
			config_.framesInFlight
	);
}

void Application::initCommandPool () {
	const auto& graphicsQueue = device_->getGraphicsQueue();
	commandPool_ = std::make_unique<de::CommandPool>(*device_, graphicsQueue.queuFamilyIndex, vk::CommandPoolCreateFlagBits::eResetCommandBuffer);
}

std::vector<const char*> Application::getRequiredExtensions() {
	std::vector<const char*> extensions;
	uint32_t extCount;
	const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&extCount);
	for (uint32_t i = 0; i < extCount; ++i) 
		extensions.push_back(glfwExtensions[i]);
	return extensions;
}

std::pair<vk::Viewport, vk::Rect2D> getViewportState(const vk::Extent2D& viewportExtent) {

	auto viewport = vk::Viewport{}
		.setX(0.f)
		.setY(0.f)
		.setWidth(static_cast<float>(viewportExtent.width))
		.setHeight(static_cast<float>(viewportExtent.height))
		.setMinDepth(0.0f)
		.setMaxDepth(1.f);

	auto scissors = vk::Rect2D{ {0,0},viewportExtent };

	return std::pair<vk::Viewport, vk::Rect2D>(viewport, scissors);
};


void Application::renderFrame() {
	renderer_->renderFrame();
}


void Application::mainLoop() {
	bool wasMinimized = false;

	while (!window_.shouldClose()) {
		glfwPollEvents();

#pragma region minimize_skip
		bool minimized = window_.isMinimized();

		if (minimized) {
			if (!wasMinimized) {           
				std::cout << "frame skipped" << std::endl;
				wasMinimized = true;
			}
			continue;                       
		}
		else {
			wasMinimized = false;           
		}
#pragma endregion skip frame while window minimized

		float deltaTime = timer_.getDeltaTime();
		float totalTime = timer_.getTotalTime();
		handleInputs(deltaTime);
		updateScene(deltaTime, totalTime);
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

void Application::updateScene(float deltaTime, float totalTime) {
	
	controlledCamera_->updateCamera(deltaTime);
	mainPass_->updateTime(deltaTime, totalTime);

}