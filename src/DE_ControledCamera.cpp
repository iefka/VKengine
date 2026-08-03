#include"DE_Window.hpp"
#include "DE_ControledCamera.hpp"


namespace de{
	void ControlledCamera::handleInputs(float deltaTime, int glfwKeyMouseLock) {

		mouseControl_.toggleMouseLock(window_.getGlfwWindow(), glfwKeyMouseLock);
		mouseControl_.rotate(cameraGameObject_, window_.getGlfwWindow());
		keyboardControl_.moveInPlaneXZ(window_.getGlfwWindow(), deltaTime, cameraGameObject_);
	}
	void ControlledCamera::updateCamera(float deltaTime) {
		camera_.setView(
			cameraGameObject_.transform.translation,
			cameraGameObject_.transform.rotation);
	}
}
