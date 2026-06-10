#ifndef _DE_APPLICATION_
#define _DE_APPLICATION_

#include"DE_Instance.hpp"
#include"DE_Device.hpp"
#include"DE_Window.hpp"

	class Application{
	public:
		Application();
		~Application() = default;

		void run();
	private:

		std::unique_ptr<de::Instance> instance_;
		std::unique_ptr<de::Device> device_;
		de::Window window_;
	};


#endif 