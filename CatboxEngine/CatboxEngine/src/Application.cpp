#include "Application.h"

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <iostream>
#include <glad/glad.h>
#include <glfw3.h>

#include "Managers/EntityManager.h"
#include "Managers/RenderManager.h"
#include "Cube.h"

// ... existing processInput unchanged ...

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
}

Application::Application(float windowWidth, float windowHeight, const char* applicationName) : window(nullptr)
{
    width = windowWidth;
    height = windowHeight;

    name = applicationName;
}

Application::~Application()
{
    CleanUp();
}

int Application::Init()
{
    if (InitGlfw() != 0)
        return -1;
    if (InitGlad() != 0)
        return -1;

    // enable depth testing for proper 3D occlusion
    glEnable(GL_DEPTH_TEST);

    // initialize the simple shader (files must exist relative to working directory)
    myShader.Initialize("./src/shaders/VertexShader.vert", "./src/shaders/FragmentShader.frag");

    if (InitImGui() != 0)
        return -1;

    return 0;
}

void Application::Run()
{
    while (!glfwWindowShouldClose(window))
    {
        processInput(GetWindow()); //If escape - exit the program

        glfwPollEvents();

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Demo window
        ImGui::Begin("Hello, Catbox!");
        ImGui::Text("This is a simple window.");
        if (ImGui::Button("Click me!"))
        {
            // create a cube and register with RenderManager
            RenderManager::Get().AddRenderable(new Cube());
        }
        ImGui::End();

        // Rendering
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        // clear both color and depth buffers
        glClearColor(0.4f, 0.3f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // draw scene
        myShader.Use();
        RenderManager::Get().RenderAll(myShader);

        // draw ImGui on top
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

void Application::CleanUp()
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

int Application::InitGlfw()
{
    if (!glfwInit())
    {
        std::cout << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 4);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);


    window = glfwCreateWindow(width, height, name, NULL, NULL);
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

int Application::InitGlad()
{
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }


    return 0;
}

int Application::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    //Setup Dear ImGui style
    ImGui::StyleColorsDark();

    //Setup platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 440");

    imguiInitialized = true;

    return 0;
}