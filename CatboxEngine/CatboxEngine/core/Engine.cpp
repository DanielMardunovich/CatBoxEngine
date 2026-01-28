#include "Engine.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <algorithm>
#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "Time.h"
#include "../graphics/Mesh.h"
#include "../resources/EntityManager.h"

// CreateCubeMesh moved to EntityManager


void Engine::OnMouseMove(double xpos, double ypos)
{
    camera.OnMouseMove(xpos, ypos);
}

void Engine::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    camera.OnMouseButton(window, button, action, mods);
}

// mouse handling moved to Camera

Engine::~Engine()
{
    Cleanup();
}

// Forward declare a static callback that will be set on the GLFW window
static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Engine* eng = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (!eng) return;
    eng->OnMouseMove(xpos, ypos);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Engine* eng = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (!eng) return;
    eng->OnMouseButton(window, button, action, mods);
}

void Engine::app()
{
    Initialize();
    
    while (!glfwWindowShouldClose(window))
    {
        Time::Update();

        Update(Time::DeltaTime());
        Render();
    }
}

// mouse movement handled in Camera

void Engine::Update(float deltaTime)
{
    // handle window-level input
    if (glfwGetKey(GetWindow(), GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(GetWindow(), true);

    // let camera handle inputs
    camera.Update(GetWindow(), deltaTime);
    glfwPollEvents();

    // UI frame and draw
    uiManager.NewFrame();
    uiManager.Draw(entityManager, spawnPosition, spawnScale, deltaTime, selectedEntityIndex, camera, useSharedCube);

}

void Engine::Render()
{
    int display_w, display_h;
    glfwGetFramebufferSize(GetWindow(), &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);

    glClearColor(0.4f, 0.3f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render ImGui
    ImGui::Render();
    
    // Render scene
    myShader.Use();

    // Use camera for view/projection
    camera.Aspect = (float)display_w / (float)display_h;
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    glm::mat4 vp = proj * view;

    // Draw all spawned entities using cubeMesh
    for (const auto& e : entityManager.GetAll())
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));
        model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));

        myShader.SetMat4("u_MVP", vp);
        myShader.SetMat4("transform", model);

        // draw entity mesh (assume mesh uploaded)
        if (e.Mesh.VAO != 0)
            e.Mesh.Draw();
    }

    
    
    uiManager.Render();

    glfwSwapBuffers(GetWindow());
}


int Engine::Initialize()
{
    if (InitGlfw() != 0)
        return -1;
    if (InitGlad() != 0)
        return -1;
    
    glEnable(GL_DEPTH_TEST);

    // initialize the simple shader (files must exist relative to working directory)
    myShader.Initialize(
    "./shaders/VertexShader.vert", 
    "./shaders/FragmentShader.frag"
    );

    // no prototype cube; entities create their own meshes on spawn

    // Initialize camera
    camera.Initialize({0,0,3}, {0,0,0}, {0,1,0}, 60.0f, width / height, 0.1f, 100.0f);

    
    if (InitImGui() != 0)
        return -1;

    return 0;
}

int Engine::InitGlfw()
{
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << '\n';
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    window = glfwCreateWindow(static_cast<int>(width), static_cast<int>(height), name, nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // Enable vsync

    // Set user pointer so callbacks can access the Engine instance
    glfwSetWindowUserPointer(window, this);

    // set mouse callbacks
    glfwSetCursorPosCallback(window, MouseCallback);
    glfwSetMouseButtonCallback(window, MouseButtonCallback);

    glfwInitialized = true;

    return 0;
}

int Engine::InitGlad()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << '\n';
        return -1;
    }

    return 0;
}

int Engine::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();

    //Setup platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 440");

    imguiInitialized = true;

    return 0;
}

void Engine::Cleanup()
{
    if (imguiInitialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        imguiInitialized = false;
    }

    if (glfwInitialized)
    {
        glfwDestroyWindow(window);
        glfwTerminate();

        glfwInitialized = false;
    }
}
