#pragma once
#include <string>
struct GLFWwindow;

class Platform
{
public:
    Platform() : window(nullptr) {}
    ~Platform() {}

    // Initialize GLFW and create a window
    bool Init(int width, int height, const char* title);
    void Shutdown();

    GLFWwindow* GetWindow() const { return window; }

private:
    GLFWwindow* window;
};
