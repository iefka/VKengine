#pragma once

#include<vulkan/vulkan.hpp>

#include"DE_Camera.hpp"
#include"DE_MovementController.hpp"
#include"DE_GameObject.hpp"

namespace de {
	class Window;

	class ControlledCamera {
	public:
		ControlledCamera(Window& window) :window_(window),
			cameraGameObject_{ GameObject::createGameObject() } {

		}

		void handleInputs(float deltaTime, int glfwKeyMouseLock);
		void updateCamera(float deltaTime);

		const Camera& getCamera()noexcept { return camera_; }

	private:
		Camera camera_{};
		GameObject cameraGameObject_;
		Window& window_;
		KeyboardMovementController keyboardControl_;
		MouseLookController mouseControl_;

	};
}