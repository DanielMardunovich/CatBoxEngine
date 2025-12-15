#pragma once
struct GLFWwindow;

#include "Shader.h"

class Application
{
public:
    Application(float windowWidth, float windowHeight, const char* applicationName);
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

    float width, height;
    const char* name;

    bool glfwInitialized = false;
    bool imguiInitialized = false;

    // added shader used for scene rendering
    Shader myShader;
};