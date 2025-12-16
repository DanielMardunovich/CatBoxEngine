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

#include "Camera.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "./Managers/InputManager.h"
#include "./Managers/UIManager.h"


// file-scope camera instance so callbacks can access it without changing headers
static Camera g_Camera(glm::vec3(0.0f, 0.0f, 3.0f));
static bool firstMouse = true;
static double lastX = 0.0, lastY = 0.0;

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
    Camera* cam = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (!cam) return;

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - lastX);
    float yoffset = static_cast<float>(lastY - ypos); // reversed: y ranges bottom->top
    lastX = xpos;
    lastY = ypos;

    cam->ProcessMouseMovement(xoffset, yoffset);
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    Camera* cam = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
    if (!cam) return;
    cam->ProcessMouseScroll(static_cast<float>(yoffset));
}

void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    Camera* cam = reinterpret_cast<Camera*>(glfwGetWindowUserPointer(window));
    // deltaTime will be applied in Run loop to camera movement calls there
    // This function will only set movement based on key state; actual movement will be applied in Run where deltaTime is known.
    // For simplicity, we do nothing here beyond escape; movement is handled in Run.
}

Application::Application(int windowWidth, int windowHeight, const char* applicationName) : window(nullptr)
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
    myShader.Initialize("src/Shaders/VertexShader.vert", "src/Shaders/FragmentShader.frag");

    // dump active uniforms and program id
    std::cout << "Shader program ID = " << myShader.myShaderProgram << std::endl;
    myShader.DumpActiveUniforms();

    // check link status explicitly (optional)
    GLint linkStatus = 0;
    glGetProgramiv(myShader.myShaderProgram, GL_LINK_STATUS, &linkStatus);
    if (linkStatus != GL_TRUE)
        std::cerr << "Warning: shader program not linked correctly (linkStatus != GL_TRUE)\n";

    // initialize ImGui via UIManager (replaces previous InitImGui usage)
    if (!UIManager::Get().Initialize(window, "#version 440"))
        return -1;

    // create a cube and register with RenderManager
    RenderManager::Get().AddRenderable(new Cube());

    // initialize centralized input handling (registers callbacks, captures cursor)
    InputManager::Get().Initialize(window, &g_Camera);

    return 0;
}

void Application::Run()
{
    float lastFrame = 0.0f;
    while (!glfwWindowShouldClose(window))
    {
        // timing
        float currentFrame = static_cast<float>(glfwGetTime());
        float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(GetWindow());

        InputManager::Get().ProcessInput(deltaTime);

        glfwPollEvents();

        // Start the Dear ImGui frame via UIManager (also lets InputManager apply pending mouse actions)
        UIManager::Get().NewFrame();

        // --- Any application logic / UI state updates happen inside UIManager ---

        // Rendering
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        // clear both color and depth buffers
        glClearColor(0.4f, 0.3f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // set projection and view uniforms
        myShader.Use();
        GLint currentProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        if (static_cast<GLuint>(currentProgram) != myShader.myShaderProgram)
        {
            myShader.Use(); // ensure correct program bound before setting uniforms
        }
        glm::mat4 projection = glm::perspective(glm::radians(g_Camera.GetFov()), (float)display_w / (float)display_h, 0.1f, 100.0f);
        glm::mat4 view = g_Camera.GetViewMatrix();
        // model matrix default (individual renderables upload their own model)
        glm::mat4 model = glm::mat4(1.0f);

        myShader.SetMat4("projection", projection);
        myShader.SetMat4("view", view);
        myShader.SetMat4("model", model);

        // draw scene
        RenderManager::Get().RenderAll(myShader);

        // draw ImGui on top via UIManager
        UIManager::Get().Render();

        glfwSwapBuffers(window);
    }
}

void Application::CleanUp()
{
    if (imguiInitialized)
    {
        // UIManager handles ImGui shutdown now
        UIManager::Get().Shutdown();

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
    // kept for compatibility if needed elsewhere — no longer used in normal init flow
    return 0;
}

int Application::InitImGui_Backup()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 440");
    imguiInitialized = true;
    return 0;
}