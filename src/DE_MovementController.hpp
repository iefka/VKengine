#pragma once

#include<GLFW/glfw3.h>
#include"glm_config.hpp"

namespace de {
	class Window;
	class GameObject;

	class KeyboardMovementController {
		struct KeyMapping {
			int MoveForward	 = GLFW_KEY_W;
			int MoveBackward = GLFW_KEY_S;
			int MoveLeft	 = GLFW_KEY_A;
			int MoveRight	 = GLFW_KEY_D;
			int MoveUP		 = GLFW_KEY_E;
			int MoveDown	 = GLFW_KEY_Q;
		};
	public:
		void moveInPlaneXZ(
			GLFWwindow* window,
			const float deltaTime,
			GameObject& gameObject);
	private:
		KeyMapping keys{};
		float moveSpeed{ 3.f };
	};

	class MouseLookController{
	public:
		void rotate(
			GameObject& gameObject,
			GLFWwindow* window);
		void toggleMouseLock(
			GLFWwindow* window,
			int Key = GLFW_KEY_C);
		float sensitivity{ 0.002f };
	private:
		bool firstMouse_{ true };
		double lastX_{ 0.f };
		double lastY_{ 0.f };
		bool mouseLocked_{ false };
		bool lockKeyWasPressed_{ false };
	};
}