#include <iostream>
#include "core/Engine.h"



int main() {

	std::cout << "Welcome to Catbox Engine!" << '\n';
	
	Engine* engine = new Engine();
	
	engine->app();
	
	delete engine;
	return 0;
}