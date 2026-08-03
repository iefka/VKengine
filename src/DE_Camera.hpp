#ifndef _DE_CAMERA_
#define _DE_CAMERA_

#include"glm_config.hpp"


namespace de{
	class Camera {
	public:
		Camera(glm::vec3 position = { 0.0f, 0.0f, -3.0f },
			float fov = { 30.0f },
			float nearPlane = { 0.1f },
			float farPlane = { 10.0f }) 
			:position_{position},
			fov_{fov},
			nearPlane_{ nearPlane },
			farPlane_{farPlane} {}

		glm::mat4 Camera::getViewMatrix() const {
			return glm::lookAt(position_, position_+lookDir_, upDir_);
		}

		glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
			auto projection = glm::perspectiveRH_ZO(glm::radians(fov_), aspectRatio, nearPlane_, farPlane_);
			projection[1][1] *= -1.0f;

			return projection;
		}

		void setView(const glm::vec3& position,const glm::vec3& rotation) {
			position_ = position;
			
			constexpr float MAX_PITCH = glm::radians(85.0f);
			constexpr float EPSILON = 1e-6f;

			//pitch limit
			glm::vec3 rot = rotation;
			rot.x = glm::clamp(rot.x, -MAX_PITCH, MAX_PITCH);

			const float yaw = rot.y;
			const float pitch = rot.x;

			//view dir
			glm::vec3 front{};
			front.x = glm::sin(yaw) * glm::cos(pitch);
			front.y = glm::sin(pitch);
			front.z = -glm::cos(yaw) * glm::cos(pitch);

			if (glm::dot(front, front) > EPSILON) {
				lookDir_ = glm::normalize(front);
			}else {
				lookDir_ = glm::vec3{ 0.f, 0.f, -1.f };
			}

			//up vector
			const glm::vec3 worldUp{ 0.f, 1.f, 0.f };
			glm::vec3 right = glm::cross(lookDir_, worldUp);

			if (glm::dot(right,right) > EPSILON) {
				right = glm::normalize(right);
			}else{
				right = glm::vec3(1.f, 0.f, 0.f);
			}
			upDir_ = glm::normalize(glm::cross(right, lookDir_));
		}


	private:
		glm::vec3 position_{};
		glm::vec3 lookDir_{0.f,0.f,-1.f};
		glm::vec3 upDir_{ 0.f,1.f,0.f };

		float fov_{};
		float nearPlane_{};
		float farPlane_{};


	};
}
#endif // !_DE_CAMERA_

