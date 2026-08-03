#include "DE_MovementController.hpp"
#include"DE_Window.hpp"
#include"DE_GameObject.hpp"

namespace de {
	//keyboardControler 
	void KeyboardMovementController::moveInPlaneXZ(
		GLFWwindow* window,
		const float deltaTime,
		GameObject& gameObject){
	
		constexpr float EPSILON = 1e-6f;

		float yaw = gameObject.transform.rotation.y;
		glm::vec3 forwardDir{ glm::sin(yaw),0.f, -glm::cos(yaw) };
		glm::vec3 rightDir{ glm::cos(yaw), 0.f, glm::sin(yaw)};
		glm::vec3 upDir{ 0.f, 1.f, 0.f };


		glm::vec3 moveDir{ 0.f };
		if (glfwGetKey(window, keys.MoveForward) == GLFW_PRESS) {
			moveDir += forwardDir;
		}
		if (glfwGetKey(window, keys.MoveBackward) == GLFW_PRESS) {
			moveDir -= forwardDir;
		}
		if (glfwGetKey(window, keys.MoveLeft) == GLFW_PRESS) {
			moveDir -= rightDir;
		}
		if (glfwGetKey(window, keys.MoveRight) == GLFW_PRESS) {
			moveDir += rightDir;
		}
		if (glfwGetKey(window, keys.MoveUP) == GLFW_PRESS) {
			moveDir += upDir;
		}
		if (glfwGetKey(window, keys.MoveDown) == GLFW_PRESS) {
			moveDir -= upDir;
		}

		if (glm::dot(moveDir, moveDir) > EPSILON) {
			gameObject.transform.translation += moveSpeed * 
				deltaTime * glm::normalize(moveDir);
		}
	}

	//mouseControl
	void MouseLookController::rotate(
		GameObject& gameObject,
		GLFWwindow* window){
		if (!mouseLocked_) { return; }

		double Xpos{}, Ypos{};

		glfwGetCursorPos(window, &Xpos, &Ypos);

		if (firstMouse_) {
			lastX_ = Xpos;
			lastY_ = Ypos;
			firstMouse_ = false;
		}

		double Xoffset = (Xpos - lastX_) * sensitivity;
		double Yoffset = (lastY_ - Ypos) * sensitivity;

		lastX_ = Xpos;
		lastY_ = Ypos;

		gameObject.transform.rotation.x += static_cast<float>(Yoffset);
		gameObject.transform.rotation.y += static_cast<float>(Xoffset);

		constexpr float MAX_PITCH = glm::radians(85.f);

		gameObject.transform.rotation.x = glm::clamp(gameObject.transform.rotation.x, -MAX_PITCH, MAX_PITCH);
		gameObject.transform.rotation.y = glm::mod(gameObject.transform.rotation.y, glm::two_pi<float>());
	}
	void MouseLookController::toggleMouseLock(GLFWwindow* window, int key) {
		bool pressed = glfwGetKey(window, key) == GLFW_PRESS; 
		
		if (pressed && !lockKeyWasPressed_) {
			mouseLocked_ = !mouseLocked_;
			glfwSetInputMode(
				window,
				GLFW_CURSOR,
				mouseLocked_ ?
				GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

			if (mouseLocked_) {
				firstMouse_ = true;
				glfwSetCursorPos(window, lastX_, lastY_);
			}
		}
		lockKeyWasPressed_ = pressed;
	}
}
