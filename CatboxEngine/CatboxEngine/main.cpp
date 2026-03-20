#include <iostream>
#include "core/Engine.h"
#include "core/MemoryTracker.h"

int main() {

	std::cout << "==================================" << std::endl;
	std::cout << "   Welcome to Catbox Engine!     " << std::endl;
	std::cout << "==================================" << std::endl;
	
	Engine* engine = new Engine(1920, 1080);
	MemoryTracker::Instance().RecordAllocation(engine, sizeof(Engine), __FILE__, __LINE__, __FUNCTION__);
	
	engine->app();
	
	MemoryTracker::Instance().RecordDeallocation(engine, __FILE__, __LINE__, __FUNCTION__);
	delete engine;

	std::cout << "\n=== Engine Shutdown ===" << std::endl;
	MemoryTracker::Instance().CheckForLeaks();

	std::cout << "\nEngine shutdown complete. Goodbye!" << std::endl;
	return 0;
}