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

    // Open a native file dialog. Returns true and writes path into outPath on success.
    // 'filter' should be a platform-specific filter string (no default provided).
    static bool OpenFileDialog(char* outPath, int maxLen, const char* filter);
    
    // Save file dialog
    static bool SaveFileDialog(char* outPath, int maxLen, const char* filter);
    
    // Install GLFW file drop callback on the given window to receive dropped file paths
    static void InstallDropCallback(GLFWwindow* window);

private:
    GLFWwindow* window;
};
