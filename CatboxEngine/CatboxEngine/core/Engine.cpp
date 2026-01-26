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

Mesh CreateCubeMesh() 
{
    Mesh mesh;

    mesh.Vertices = {
        // Front (+Z)
        {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0, 0}},
        {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0, 0}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1, 0}},
        {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1, 0}},

        // Back (-Z)
        {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 0, 0}},
        {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 0, 0}},
        {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1, 0}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 1, 0}},

        // Left (-X)
        {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0, 0}},
        {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 0, 0}},
        {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1, 0}},
        {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 1, 0}},

        // Right (+X)
        {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0, 0, 0}},
        {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 0, 0}},
        {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 1, 0}},
        {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0, 1, 0}},

        // Top (+Y)
        {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0, 0, 0}},
        {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 0, 0}},
        {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1, 0}},
        {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0, 1, 0}},

        // Bottom (-Y)
        {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 0, 0}},
        {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 0, 0}},
        {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 1, 0}},
        {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 1, 0}},
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

void Engine::OnMouseButton(int button, int action, int mods)
{
    if (button != GLFW_MOUSE_BUTTON_LEFT)
        return;

    if (action == GLFW_PRESS)
    {
        // capture/hide cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        firstMouse = true; // reset so we don't get a jump
    }
    else if (action == GLFW_RELEASE)
    {
        // release/show cursor
        glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        firstMouse = true;
    }
}

void processInput(GLFWwindow* window, Camera& camera, float deltaTime)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // Camera movement (WASD)
    float speed = 2.5f * deltaTime;
    glm::vec3 camPos(camera.Position.x, camera.Position.y, camera.Position.z);
    glm::vec3 camFront(camera.Front.x, camera.Front.y, camera.Front.z);
    glm::vec3 camUp(camera.Up.x, camera.Up.y, camera.Up.z);

    glm::vec3 forward = glm::normalize(camFront);
    glm::vec3 right = glm::normalize(glm::cross(forward, camUp));

    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
    {
        camPos += forward * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
    {
        camPos -= forward * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
    {
        camPos -= right * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
    {
        camPos += right * speed;
    }

    // Up and down
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
    {
        camPos += camUp * speed;
    }
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
    {
        camPos -= camUp * speed;
    }

    camera.Position = { camPos.x, camPos.y, camPos.z };
    // Keep Target in sync with front for compatibility
    camera.Target = { camera.Position.x + camera.Front.x, camera.Position.y + camera.Front.y, camera.Position.z + camera.Front.z };
}

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
    eng->OnMouseButton(button, action, mods);
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

void Engine::OnMouseMove(double xpos, double ypos)
{
    if (firstMouse)
    {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float xoffset = (float)xpos - lastX;
    float yoffset = lastY - (float)ypos; // reversed since y-coordinates go from bottom to top

    lastX = (float)xpos;
    lastY = (float)ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

void Engine::Update(float deltaTime)
{
    processInput(GetWindow(), camera, deltaTime);
    glfwPollEvents();

    // Start ImGui frame (logic)
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // UI logic
    ImGui::Begin("Hello, Catbox!");
    ImGui::Text("This is a simple window.");

    ImGui::Separator();
    ImGui::Text("Spawn Cube");
    ImGui::InputFloat3("Position", &spawnPosition.x);
    ImGui::InputFloat3("Scale", &spawnScale.x);
    if (ImGui::Button("Spawn"))
    {
        Entity e;
        e.name = "Cube";
        e.Mesh = cubeMesh;
        e.Transform.Position = spawnPosition;
        e.Transform.Scale = spawnScale;
        entities.push_back(e);
        std::cout << "Spawned cube at " << spawnPosition.x << "," << spawnPosition.y << "," << spawnPosition.z << '\n';
    }

    // Display timing
    ImGui::Separator();
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
    
    // Render scene
    myShader.Use();

    // Use camera for view/projection
    camera.Aspect = (float)display_w / (float)display_h;
    glm::mat4 view = camera.GetViewMatrix();
    glm::mat4 proj = camera.GetProjectionMatrix();
    glm::mat4 vp = proj * view;

    // Draw all spawned entities using cubeMesh
    for (const auto& e : entities)
    {
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(e.Transform.Position.x, e.Transform.Position.y, e.Transform.Position.z));
        model = glm::rotate(model, (float)glfwGetTime(), glm::vec3(1.0f, 0.0f, 1.0f));
        model = glm::scale(model, glm::vec3(e.Transform.Scale.x, e.Transform.Scale.y, e.Transform.Scale.z));

        myShader.SetMat4("u_MVP", vp);
        myShader.SetMat4("transform", model);

        // draw cube mesh
        if (e.Mesh.VAO != 0)
            e.Mesh.Draw();
        else
            cubeMesh.Draw();
    }

    
    
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

    // prepare prototype cube mesh
    cubeMesh = CreateCubeMesh();
    cubeMesh.Upload();

    // Initialize camera
    camera.SetPosition({0,0,3});
    camera.SetTarget({0,0,0});
    camera.SetUp({0,1,0});
    camera.SetPerspective(60.0f, width / height, 0.1f, 100.0f);

    // initialize mouse coordinates
    int w = static_cast<int>(width);
    int h = static_cast<int>(height);
    lastX = w / 2.0f;
    lastY = h / 2.0f;

    
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
