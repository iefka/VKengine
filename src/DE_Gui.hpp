#include<memory>

#include"third_party/imgui/imgui.h"


namespace de {
	class Texture;
	class Device;

	class GUI {
	public:
		GUI(const Device& device);
		~GUI();
	private:
		std::unique_ptr<Texture> fontTexture_;
		std::unique_ptr<ImDrawData> drawData;
	};
	
}