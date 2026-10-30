
#include "DE_Application.hpp"
#include "DE_Memory.hpp"
#include "DE_Shaders.hpp"
#include "DE_Pipeline.hpp"
#include "DE_Renderer.hpp"
#include "DE_CommandBuffers.hpp"
#include "DE_PushConstants.hpp"
#include "Utility/DE_Debug.hpp"
#include"DE_MaterialBuffer.hpp"
#include"DE_BindlessTextureRegistry.hpp"
#include"DE_IndirectCommandBuffer.hpp"
#include"SSBO/DE_TransformBuffer.hpp"
#include "Utility/DE_Utility.hpp"

#include"third_party/imgui/imgui.h"
#include"third_party/imgui/backends/imgui_impl_vulkan.h"
#include"third_party/imgui/backends/imgui_impl_glfw.h"


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


void FrameCounter::tick(float deltaTime){

	++numFrames_;
	accumulatedTime_ += deltaTime;

	if (accumulatedTime_ < avgIntervalSec_){return;}

	currentFPS_ = static_cast<float>(
		numFrames_ / accumulatedTime_
		);
	numFrames_ = 0;
	accumulatedTime_ = 0;
}

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
	descriptorLayouts_.try_emplace("ubo",de::DescriptorSetLayout::Builder{*device_}
		.addBinding(0, vk::DescriptorType::eUniformBuffer, vk::ShaderStageFlagBits::eAllGraphics)
		.build()
	);

	//set 1

	

}

void Application::preparePasses(){

	mainPass_ = std::make_unique<de::MainRenderPass>(*device_,
		*renderer_,
		*skyboxSystem_,
		*renderSystem_,
		*pointLightSystem_,
		*controlledCamera_,
		gameObjects,
		descriptorLayouts_.at("ubo")
		);

	guiPass_ = std::make_unique<de::GUIRenderPass>(
		*instance_,
		*device_,
		window_,
		*renderer_
	);

#if 0
	shaderTexturingPass_ = std::make_unique<de::ShaderTexturingPass>(*device_,
		window_.getExtent(), renderer_->getMainRenderPass(),
		*renderer_, "Shaders/bin/compute.comp.spv");

	renderer_->registerPass(shaderTexturingPass_.get());
#endif

	renderer_->registerPass(mainPass_.get());
	renderer_->registerPass(guiPass_.get());

}

void Application::initGameObjects() {

	constexpr uint32_t kMaxTextures = 1024u;
	constexpr uint32_t kMaterialInitialCapacity = 256u;

	bindlessRegistry_ = std::make_unique<de::BindlessTextureRegistry>(*device_, kMaxTextures);
	materialBuffer_ = std::make_unique<de::MaterialBuffer>(*device_, kMaterialInitialCapacity);
	materialManager_ = std::make_unique<de::MaterialManager>(*device_, *bindlessRegistry_, *materialBuffer_);
	transformBuffer_ = std::make_unique<de::TransformBuffer>(*device_, config_.framesInFlight);
	indirectBuffer_ = std::make_unique<de::IndirectCommandBuffer>(*device_, config_.framesInFlight);
	geometryPool = std::make_unique<de::GeometryPool>(*device_);

	controlledCamera_ = std::make_unique<de::ControlledCamera>(window_);

	auto smoothVaseModel = de::Mesh::loadFromFile(*device_, *geometryPool,"Models/smooth_vase.obj");
	if (!smoothVaseModel) {
		std::cout << smoothVaseModel.error();
	}
	auto smoothVase = de::GameObject::createGameObject();
	smoothVase.mesh = std::move(smoothVaseModel->mesh);
	smoothVase.materialsByMesh_ = materialManager_->resolveMaterials(
		smoothVaseModel->materials,
		smoothVaseModel->defaultMaterialIDs,
		smoothVaseModel->mtlBaseDir
	);
	smoothVase.transform.rotation.x = glm::radians(180.f);
	smoothVase.transform.translation = { .0f, .0f, 0.f };
	smoothVase.transform.scale = glm::vec3(3.f);
	gameObjects.emplace(smoothVase.getId(), std::move(smoothVase));

#if 1
	auto galleryModel = de::Mesh::loadFromFile(*device_, *geometryPool, "assets/gallery/gallery.obj");
	auto gallery = de::GameObject::createGameObject();
	gallery.mesh = std::move(galleryModel->mesh);
	gallery.materialsByMesh_ = materialManager_->resolveMaterials(
		galleryModel->materials,
		galleryModel->defaultMaterialIDs,
		galleryModel->mtlBaseDir
	);
	gameObjects.emplace(gallery.getId(), std::move(gallery));
#endif

#if BEDROOM
	auto bedroomModel = de::Mesh::loadFromFile(*device_, *geometryPool, "assets/bedroom/iscv2.obj");
	auto bedroom = de::GameObject::createGameObject();
	bedroom.mesh = std::move(bedroomModel->mesh);
	bedroom.materialsByMesh_ = materialManager_->resolveMaterials(
		bedroomModel->materials,
		bedroomModel->defaultMaterialIDs,
		bedroomModel->mtlBaseDir
	);
	bedroom.transform.scale = glm::vec3(0.2f);
	bedroom.transform.rotation.x = glm::radians(-90.f);
	gameObjects.emplace(bedroom.getId(), std::move(bedroom));

#endif
#if 1
	auto cubeModel = de::Mesh::loadFromFile(*device_, *geometryPool, "Models/cube.obj");
	auto cube = de::GameObject::createGameObject();
	cube.mesh = std::move(cubeModel->mesh);
	cube.materialsByMesh_ = materialManager_->resolveMaterials(
		cubeModel->materials,
		cubeModel->defaultMaterialIDs,
		cubeModel->mtlBaseDir
	);
	cube.transform.rotation.x = glm::radians(180.f);
	cube.transform.scale =glm::vec3(0.2f);
	cube.transform.translation = { 1.f, 0.2f, 0.f };
	gameObjects.emplace(cube.getId(), std::move(cube));


#endif

#if MODELS
	std::unique_ptr<de::Model>flatVaseModel = de::Model::createModelFromFile(*device_, "Models/flat_vase.obj", *materialManager_);
	auto flatVase = de::GameObject::createGameObject();	
	flatVase.model = std::move(flatVaseModel);
	flatVase.transform.rotation.x = glm::radians(180.f);
	flatVase.transform.translation = { .6f, .0f, -0.8f };
	flatVase.transform.scale = glm::vec3(3.f);
	gameObjects.emplace(flatVase.getId(), std::move(flatVase));

	

	std::unique_ptr<de::Model> quadModel = de::Model::createModelFromFile(*device_, "Models/quad.obj", *materialManager_);
	auto flor = de::GameObject::createGameObject();
	flor.model = std::move(quadModel);
	flor.transform.rotation.x = glm::radians(180.f);
	flor.transform.scale = { 3.f, 1.f, 3.f };
	flor.transform.translation = {0.f, 0.0f, 0.f};
	gameObjects.emplace (flor.getId(), std::move(flor));
#endif

	std::vector<glm::vec3> lightColors{
	  {1.f, .1f, .1f},
	  {.1f, .1f, 1.f},
	  {.1f, 1.f, .1f},
	  {1.f, 1.f, .1f},
	  {.1f, 1.f, 1.f},
	  {1.f, 1.f, 1.f}  //
	};

	for (int i = 0; i < lightColors.size(); i++) {
		auto pointLight = de::GameObject::makePointLight(14.f,0.1f);
		pointLight.color = lightColors[i];
		auto rotateLight = glm::rotate(
			glm::mat4(1.f),
			(i * glm::two_pi<float>()) / lightColors.size(),
			{ 0.f, 1.f, 0.f });
		pointLight.transform.translation = glm::vec3(rotateLight * glm::vec4(1.f, 4.f, 1.f, 1.f));
		gameObjects.emplace(pointLight.getId(), std::move(pointLight));
	}
}

void Application::initSystems(){

	//renderSystem
	std::vector<vk::DescriptorSetLayout> renderSystemLayouts{
		descriptorLayouts_.at("ubo").getLayout(),
		bindlessRegistry_->getDescriptorLayout(),
		materialBuffer_->getDescriptorLayout().getLayout(),
		transformBuffer_->getDescriptorLayout().getLayout()
	};
	renderSystem_ = std::make_unique<de::RenderSystem>(
		*device_, renderer_->getMainRenderPass(), window_.getExtent(), renderSystemLayouts
	);
	renderSystem_->setGeometryPool(*geometryPool);
	renderSystem_->setDefaultMaterial(materialManager_->getDefaultMaterial());
	renderSystem_->setMaterialDescriptorSet(bindlessRegistry_->getDescriptorSet());
	renderSystem_->setMaterialBufferDescriptorSet(materialBuffer_->getDescriptorSet());
	renderSystem_->setIndirectBuffer(*indirectBuffer_);
	renderSystem_->setTransformBuffer(*transformBuffer_);

	//pointLights
	std::vector<vk::DescriptorSetLayout> pointLightSystemLayout{
		descriptorLayouts_.at("ubo").getLayout()
	};
	pointLightSystem_ = std::make_unique<de::PointLightSystem>(
		*device_, renderer_->getMainRenderPass(), window_.getExtent(), pointLightSystemLayout
	);

	//skybox
	std::vector<vk::DescriptorSetLayout> skyboxSystemLayout{
		descriptorLayouts_.at("ubo").getLayout()
	};
	skyboxSystem_ = std::make_unique<de::SkyBoxSystem>(
		*device_, renderer_->getMainRenderPass(), window_.getExtent(), skyboxSystemLayout,
		*globalPool_, "Materials/Textures/Cubemap_Sky_01.png"
	);
}

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

void Application::drawUI(){

	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	
	//there my ui
	std::string fps("FPS: " + std::to_string(fpsCounter.getFPS()));

	ImGui::StyleColorsClassic();
	
	ImGui::Text(fps.c_str());

	//end
	ImGui::Render();

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

	drawUI();
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
		fpsCounter.tick(deltaTime);

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
#if 0
	shaderTexturingPass_->updateTime(totalTime);
#endif
	controlledCamera_->updateCamera(deltaTime);
	mainPass_->updateTime(deltaTime, totalTime);

}


