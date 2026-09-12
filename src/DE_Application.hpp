#ifndef _DE_APPLICATION_
#define _DE_APPLICATION_
    

#include "DE_Instance.hpp"
#include "DE_Device.hpp"
#include "DE_Window.hpp"
#include "DE_Memory.hpp"
#include "DE_Renderer.hpp"
#include "Systems/DE_RenderSystem.hpp"
#include"Systems/PointLightSystem.hpp"
#include "DE_CommandBuffers.hpp"
#include "DE_Descriptors.hpp"
#include "DE_ControledCamera.hpp"
#include"DE_GameObject.hpp"
#include "Utility/DE_Debug.hpp"
#include "Utility/DE_Utility.hpp"
#include"glm_config.hpp"
#include"MaterialManager.hpp"
#include"DE_MainRenderPass.hpp"


#include <vector>
#include <memory>
#include <chrono>


class AppTimer {
public:
	AppTimer();
	void reset();
	float getDeltaTime();
	float getTotalTime() const;
private:
	std::chrono::steady_clock::time_point startTime_;
	std::chrono::steady_clock::time_point lastFrameTime_;
	float totalTime_{0.0f};
	float deltaTime_{0.0f};
};

struct ApplicationConfig {
	int windowWidth = 800;
	int windowHeight = 600;
	const char* windowTitle = "MyEngine";
	uint32_t swapchainImageCount = 2;
	uint32_t framesInFlight = 2;

	// Vulkan metadata
	const char* applicationName = "Dimasik";
	uint32_t applicationVersion = 1;
	const char* engineName = "none";
	uint32_t engineVersion = 1;
};

class Application {
public:
	Application(const ApplicationConfig& config = ApplicationConfig());
	~Application();

	void run();

private:
	// init
	void initVulkan();
	void initResources();
	void initGameObjects();
	void initRenderer();
	void initSystems();
	void initCommandPool();
	void initDescriptorSets();
	void preparePasses();

	// helpers
	std::vector<const char*> getRequiredExtensions();
	void updateScene(float deltaTime, float totalTime);
	void renderFrame();
	void handleInputs(float deltaTime);
	void mainLoop();
	void cleanup();

private:
	ApplicationConfig config_;
	std::unique_ptr<de::Instance> instance_;
	std::unique_ptr<de::Device> device_;
	de::Window window_;
	

	// resources
	std::unique_ptr<de::DescriptorPool> globalPool_;
	std::vector<de::DescriptorSetLayout> descriptorLayouts_;
	std::unique_ptr<de::MaterialManager> materialManager_;
	de::GameObject::Map gameObjects;
	std::unique_ptr<de::MainRenderPass> mainPass_;

	std::unique_ptr<de::RenderSystem> renderSystem_;
	std::unique_ptr<de::PointLightSystem> pointLightSystem_;
	std::unique_ptr<de::CommandPool> commandPool_;
	std::unique_ptr<de::Renderer> renderer_;
	std::unique_ptr<de::ControlledCamera> controlledCamera_;

	AppTimer timer_;
};

#endif // _DE_APPLICATION_
