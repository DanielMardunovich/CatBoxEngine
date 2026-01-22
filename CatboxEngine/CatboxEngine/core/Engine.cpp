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

#include "Time.h"
#include "../graphics/Mesh.h"

Mesh CreateCubeMesh() 
{
    Mesh mesh;

    mesh.Vertices = {
        // positions              // normals         // uv (unused for now)
        {{-0.5f,-0.5f,-0.5f}, {0,0,-1}, {0,0,0}},
        {{ 0.5f,-0.5f,-0.5f}, {0,0,-1}, {1,0,0}},
        {{ 0.5f, 0.5f,-0.5f}, {0,0,-1}, {1,1,0}},
        {{-0.5f, 0.5f,-0.5f}, {0,0,-1}, {0,1,0}},

        {{-0.5f,-0.5f, 0.5f}, {0,0,1}, {0,0,0}},
        {{ 0.5f,-0.5f, 0.5f}, {0,0,1}, {1,0,0}},
        {{ 0.5f, 0.5f, 0.5f}, {0,0,1}, {1,1,0}},
        {{-0.5f, 0.5f, 0.5f}, {0,0,1}, {0,1,0}},
    };

    mesh.Indices = {
        0,1,2, 2,3,0,
        4,5,6, 6,7,4,
        0,4,7, 7,3,0,
        1,5,6, 6,2,1,
        3,2,6, 6,7,3,
        0,1,5, 5,4,0
    };

    return mesh;
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

Engine::~Engine()
{
    Cleanup();
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

void Engine::Update(float deltaTime)
{
    processInput(GetWindow());
    glfwPollEvents();

    // Start ImGui frame (logic)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // UI logic
    ImGui::Begin("Hello, Catbox!");
    ImGui::Text("This is a simple window.");
    if (ImGui::Button("Click me!"))
    {
        std::cout << "cube created" << '\n';
    }

    //Displaying time and delta time
    ImGui::Text("Delta: %.4f", Time::DeltaTime());
    ImGui::Text("FPS: %.1f", 1.0f / Time::DeltaTime());

    ImGui::End();

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
    
    //Render scene
    myShader.Use();
    
    glm::mat4 model = glm::translate(glm::mat4(1.0f),
        glm::vec3(
            cubeEntity.Transform.Position.x,
            cubeEntity.Transform.Position.y,
            cubeEntity.Transform.Position.z
        )
    );
    
    glm::mat4 view = glm::lookAt(
        glm::vec3(0, 0, 3),
        glm::vec3(0, 0, 0),
        glm::vec3(0, 1, 0)
    );
    
    glm::mat4 proj = glm::perspective(
        glm::radians(60.0f),
        (float)display_w / (float)display_h,
        0.1f,
        100.0f
    );
    
    glm::mat4 mvp = proj * view * model;
    myShader.SetMat4("u_MVP", mvp);
    
    if (cubeEntity.Mesh.VAO == 0)
    {
        std::cerr << "ERROR: Cube VAO is 0\n";
    }
    
    cubeEntity.Mesh.Draw();

    
    
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

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

    cubeEntity.name = "Cube";
    cubeEntity.Mesh = CreateCubeMesh();
    cubeEntity.Mesh.Use();

    
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
