#include <iostream>
#include "core/Engine.h"
#include "core/MemoryTracker.h"

int main() {

	std::cout << "==================================" << std::endl;
	std::cout << "   Welcome to Catbox Engine!     " << std::endl;
	std::cout << "==================================" << std::endl;
	
	Engine* engine = TRACKED_NEW_ARGS(Engine, 1920, 1080);
	
	engine->app();
	
	TRACKED_DELETE(engine);
	std::cout << "\nEngine shutdown complete. Goodbye!" << std::endl;
	return 0;
}