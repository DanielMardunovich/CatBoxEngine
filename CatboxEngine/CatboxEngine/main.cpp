#include <iostream>
#include "core/Engine.h"



int main() {

	std::cout << "==================================" << std::endl;
	std::cout << "   Welcome to Catbox Engine!     " << std::endl;
	std::cout << "==================================" << std::endl;
	
	Engine* engine = new Engine(1920, 1080);
	
	engine->app();
	
	delete engine;
	std::cout << "\nEngine shutdown complete. Goodbye!" << std::endl;
	return 0;
}