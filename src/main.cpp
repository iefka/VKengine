  #include "DE_Application.hpp"
#include<Windows.h>
//std
#include<iostream>

int main()
{
	Application app{};

	try{
		app.run();
	} catch (std::exception& ex){
		std::cout <<"Exception thrown: " << ex.what() << '\n';
		return  EXIT_FAILURE;
	}
	return  EXIT_SUCCESS;

}