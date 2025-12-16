#pragma once
struct GLFWwindow;

#include "Shader.h"

class Application
{
public:
    Application(int windowWidth, int windowHeight, const char* applicationName);
    ~Application();

    int Init();

    void Run();

    void CleanUp();

    GLFWwindow* GetWindow() { return window; }

private:
    int InitGlfw();
    int InitGlad();
    int InitImGui();

    GLFWwindow* window;

    int width, height;
    const char* name;

    bool glfwInitialized = false;
    bool imguiInitialized = false;

    // added shader used for scene rendering
    Shader myShader{}; // <-- ensure myShader is value-initialized
};