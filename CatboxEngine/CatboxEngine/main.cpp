#include "src/Application.h"
#include <iostream>

int main() {

	std::cout << "Welcome to Catbox Engine!" << std::endl;

	Application* app = new Application(640, 820, "Catbox Engine");

	app->Init();

	app->Run();

	app->CleanUp();

	delete app;
	return 0;
}