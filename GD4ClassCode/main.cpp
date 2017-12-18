#include "Application.hpp"

#include <stdexcept>
#include <iostream>
Application app;

int main()
{
	try {
		app.run();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
		std::cin.ignore();
	}
}