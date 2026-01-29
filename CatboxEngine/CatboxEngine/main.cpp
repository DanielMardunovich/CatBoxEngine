#include <iostream>
#include "core/Engine.h"



int main() {

	std::cout << "Welcome to Catbox Engine!" << '\n';
	
	Engine* engine = new Engine(1920, 1080);
	
	engine->app();
	
	delete engine;
	return 0;
}