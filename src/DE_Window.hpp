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
		
		GLFWwindow* getGlfwWindow() { return glfwWindow_.get(); }

		vk::SurfaceFormatKHR getSurfaceFormat() const noexcept {
			return selectedSurfaceFormat_;
		}

		void createSurface(const Instance& instance);


		//if suport select prefered format and store it, else select first suported
		void selectSurfaceFormat(const Device& device, vk::SurfaceFormatKHR preferedSurfaceFormat);

		const vk::UniqueSurfaceKHR& getSurface() const noexcept {
			return surface_;
		}

		void endResize()  {
			windowResized = false;
		}
		bool isResized() const {
			return windowResized;
		}
		bool isMinimized() const {
			return windowMinimized;
		}
		 const vk::Extent2D getExtent() const noexcept {
			return { width_ ,height_ };
		}

	private:
		std::vector<vk::SurfaceFormatKHR> requestWindowFormat(const Device& device);

		static void frameBufferResizeCallback(GLFWwindow* window, int width, int height);

		void onFramebufferSizeChanged(int width, int height);
		window_ptr_t  initAndCreateWindow(uint16_t width, uint16_t height, const char* windowTitle);

		uint16_t width_;
		uint16_t height_;

		window_ptr_t glfwWindow_;
		bool windowMinimized = false;
		bool windowResized = false;

		vk::UniqueSurfaceKHR surface_;
		vk::SurfaceFormatKHR selectedSurfaceFormat_;
	};
}
#endif // !1