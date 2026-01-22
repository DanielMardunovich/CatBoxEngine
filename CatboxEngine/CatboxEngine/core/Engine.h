#pragma once
#include "../graphics/Shader.h"
#include "../resources/Entity.h"

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
};
