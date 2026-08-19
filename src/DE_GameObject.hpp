#ifndef _DE_GameObject_
#define _DE_GameObject_

#include"DE_Model.hpp"

#include"glm_config.hpp"
#include<unordered_map>
#include<memory> 


namespace de {

	struct TransformComponent {
	public:
		glm::vec3 translation{};
		glm::vec3 scale{ 1.f,1.f,1.f };
		glm::vec3 rotation{};

		glm::mat4 mat4() const noexcept {
			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, translation);
			model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
			model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
			model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
			model = glm::scale(model, scale);
			return model;
		}
		glm::mat3 normalMatrix()const noexcept {

			glm::mat4 model = glm::mat4(1.0f);
			model = glm::translate(model, translation);
			model = glm::rotate(model, rotation.x, glm::vec3(1, 0, 0));
			model = glm::rotate(model, rotation.y, glm::vec3(0, 1, 0));
			model = glm::rotate(model, rotation.z, glm::vec3(0, 0, 1));
			model = glm::scale(model, scale);

			glm::mat3 model3 = glm::mat3(model);
			return glm::transpose(glm::inverse(model3));

		}
	};


	class GameObject {
	public:

		using Map = std::unordered_map<uint32_t, GameObject>;

		uint32_t getId() { return id_; }
		
		static GameObject createGameObject() {
			static uint32_t currentID = 0;
				return GameObject(currentID++);
		}

		GameObject(const GameObject&) = delete;
		GameObject& operator= (const GameObject&) = delete;
		GameObject(GameObject&&) = default;
		GameObject& operator= (GameObject&&) = default;

		std::unique_ptr<Model> model{};
		glm::vec3 color{};
		TransformComponent transform{};
	private:
		GameObject(uint32_t ID) : id_{ID} {}
		uint32_t id_;

	};
}
#endif // !_DE_GameObject_
