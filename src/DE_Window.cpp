#include"DE_Instance.hpp"
#include"DE_Device.hpp"
#include "DE_Window.hpp"
#include<iostream>


namespace de {


	Window::Window(uint16_t width, uint16_t height, const char* windowTitle)
	:glfwWindow_{ initAndCreateWindow(width,height,windowTitle)}, width_{width}, height_{height} {
		glfwSetWindowUserPointer(glfwWindow_.get(), this);
		glfwSetFramebufferSizeCallback(glfwWindow_.get(), frameBufferResizeCallback);
		glfwSetInputMode(glfwWindow_.get(), GLFW_CURSOR, GLFW_CURSOR_NORMAL);
	}

	Window::~Window(){
		glfwTerminate();
	}


	void Window::frameBufferResizeCallback(GLFWwindow* window, int width, int height){
		Window* self = static_cast<Window*>(glfwGetWindowUserPointer(window));
		if (self) {
			self->onFramebufferSizeChanged(width, height);
		}
	}

	void Window::onFramebufferSizeChanged(int width, int height){
		windowResized = true;
		windowMinimized = (width == 0 && height == 0);

		width_ = width;
		height_ = height;
	}

	// create surface inside class
	void Window::createSurface(const Instance& instance){
		const auto& instance_vk = instance.getInstance();
		
		VkSurfaceKHR surface;

		if ( auto result = glfwCreateWindowSurface(instance_vk, glfwWindow_.get(), nullptr, &surface);
			result != VK_SUCCESS) {
			throw std::runtime_error("failed to create window surface");
		}
		vk::detail::ObjectDestroy<vk::Instance, VULKAN_HPP_DEFAULT_DISPATCHER_TYPE> deleter{ instance_vk };
		
		surface_ = vk::UniqueSurfaceKHR(vk::SurfaceKHR(surface), deleter);
	}

	void Window::selectSurfaceFormat(const Device& device, vk::SurfaceFormatKHR preferedSurfaceFormat){
	
		auto suportedFormats = requestWindowFormat(device);

		if (suportedFormats.empty()) {
			throw std::runtime_error("no suported format found!");
		}

		auto it = std::find_if(suportedFormats.begin(), suportedFormats.end(),
			[preferedSurfaceFormat](vk::SurfaceFormatKHR currentFormat) {
				return preferedSurfaceFormat == currentFormat;
			});
			
		if (it == suportedFormats.end()) {
			std::cout << "Warning!: prefered surface format is not suported, selected first suported: " <<
				vk::to_string(suportedFormats[0].format)<<std::endl;
			selectedSurfaceFormat_ = suportedFormats[0];
			return;
		}
		selectedSurfaceFormat_ = *it;
	}

	std::vector<vk::SurfaceFormatKHR> Window::requestWindowFormat(const Device& device) {
		const auto& physicalDevice = device.getPhysicalDevice();

		return physicalDevice.getSurfaceFormatsKHR(*surface_);
	}

	window_ptr_t Window::initAndCreateWindow(uint16_t width, uint16_t height, const char* windowTitle)
	{
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

		return { 
		glfwCreateWindow(width, height, windowTitle, nullptr, nullptr),
		glfwDestroyWindow };
	}
}
