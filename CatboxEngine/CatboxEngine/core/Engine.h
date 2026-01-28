#pragma once
#include "../graphics/Shader.h"
#include "../resources/Entity.h"
#include "../resources/Camera.h"
#include "../resources/EntityManager.h"
#include "UIManager.h"
#include <vector>
#include "../resources/Math/Vec3.h"

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
    void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    
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
    
    // spawned entities
    EntityManager entityManager; // Updated to handle mesh assignment directly
    UIManager uiManager;
    bool useSharedCube = true;

    Camera camera;
    // UI spawn parameters
    Vec3 spawnPosition{0,0,0};
    Vec3 spawnScale{0.5f,0.5f,0.5f};
    int selectedEntityIndex = -1;
    // mouse state moved to Camera
};
