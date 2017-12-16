#include "Application.hpp"

#include <stdexcept>
#include <iostream>

int main()
{
	try {
		Application app;
		app.run();
	}
	catch (std::exception& e)
	{
		std::cout << "Exception: " << e.what() << std::endl;
		std::cin.ignore();
	}
}