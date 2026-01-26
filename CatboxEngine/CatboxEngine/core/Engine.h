#pragma once
#include "../graphics/Shader.h"
#include "../resources/Entity.h"
#include "../resources/Camera.h"

struct GLFWwindow;

class Engine
{
public:
    Engine(float windowWidth = 800.f, float windowHeight = 800.f, const char* name = "Catbox Engine")
        : window(nullptr),
          width(windowWidth),
          height(windowHeight),
          name(name),
          glfwInitialized(false),
          imguiInitialized(false)
    {
        
    }
    
    ~Engine();

    void app();
    void OnMouseMove(double xpos, double ypos);
    
private:
    void Update(float deltaTime);
    void Render();
    
    int Initialize();
    
    int InitGlfw();
    int InitGlad();
    int InitImGui();
    
    void Cleanup();

    GLFWwindow* GetWindow() { return window; }
    
    //------------- variables -----------------
private:
    GLFWwindow* window;

    float width, height;
    const char* name;

    bool glfwInitialized;
    bool imguiInitialized;
    
    Shader myShader;
    
    Entity cubeEntity;

    Camera camera;
    // Mouse look state
    float lastX = 0.0f;
    float lastY = 0.0f;
    bool firstMouse = true;
};
