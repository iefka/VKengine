#ifndef _DE_WINDOW_
#define _DE_WINDOW_

//glfw
#define GLFW_NO_API
#define GLFW_INCLUDE_VULKAN
#include<GLFW/glfw3.h>
//memory
#include<memory>
#include<vector>
//vk
#include<vulkan/vulkan.hpp>

using window_ptr_t = std::unique_ptr<GLFWwindow, decltype(&glfwDestroyWindow)>;

namespace de {
	class Device;
	class Instance;
	
	class Window {
	public:
		Window(uint16_t width, uint16_t height, const char* windowTitle);
		~Window();

		Window(const Window&) = delete;
		Window& operator=(const Window&) = delete;

		Window(Window&&) = delete;
		Window operator=(Window&&) = delete;
		 
		bool shouldClose() noexcept {
			return glfwWindowShouldClose(glfwWindow_.get());
		}
		

		void createSurface(const Instance& instance);
		//requset formats from device and store into class
		void requestWindowFormat(const Device& device);

		const vk::UniqueSurfaceKHR& getSurface() const noexcept {
			return surface_;
		}
		const std::vector<vk::SurfaceFormatKHR>& getSurfaceFormats() {
			return surfaceFormats_;
		}
		void endResize() {
			windowResized = false;
		}
		bool isResized() {
			return windowResized;
		}
		bool isMinimized() {
			return windowMinimized;
		}
		 const vk::Extent2D getExtent() const noexcept {
			return { width_ ,height_ };
		}

	private:

		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

		void onFramebufferSizeChanged(int width, int height);
		window_ptr_t  initAndCreateWindow(uint16_t width, uint16_t height, const char* windowTitle);

		uint16_t width_;
		uint16_t height_;

		window_ptr_t glfwWindow_;
		bool windowMinimized = false;
		bool windowResized = false;

		vk::UniqueSurfaceKHR surface_;
		std::vector<vk::SurfaceFormatKHR> surfaceFormats_;
	};
}
#endif // !1