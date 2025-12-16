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

    if (InitImGui() != 0)
        return -1;

    // register camera pointer and callbacks
    // glfwSetWindowUserPointer(window, &g_Camera);
    // glfwSetCursorPosCallback(window, mouse_callback);
    // glfwSetScrollCallback(window, scroll_callback);
    // Optionally hide the cursor while controlling camera:
    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
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

        // Start the Dear ImGui frame
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Let InputManager apply any pending mouse-button actions now that ImGui IO is updated.
        InputManager::Get().FrameUpdate();

        // Entities editor window
        ImGui::Begin("Entities");
        if (ImGui::Button("Create Entity"))
        {
            EntityManager::Get().CreateEntity();
        }

        ImGui::Separator();
        auto list = EntityManager::Get().GetAllEntities();
        static Entity* selected = nullptr;
        int idx = 0;
        for (Entity* e : list)
        {
            char label[64];
            snprintf(label, sizeof(label), "%s##%d", e->name.c_str(), idx);
            if (ImGui::Selectable(label, selected == e))
                selected = e;
            idx++;
        }

        if (selected)
        {
            ImGui::Separator();
            ImGui::Text("Editing: %s", selected->name.c_str());

            if (ImGui::Button("Delete Entity"))
            {
                // Remove entity and its renderable, then clear selection.
                EntityManager::Get().DestroyEntity(selected);
                selected = nullptr;
            }
            else
            {
                // Edit only when entity still exists (Delete above clears selected).
                    // Name (char buffer)
                char nameBuf[256];
                strncpy_s(nameBuf, selected->name.c_str(), sizeof(nameBuf));
                nameBuf[sizeof(nameBuf) - 1] = '\0';
                if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
                    selected->name = std::string(nameBuf);

                // Model path
                char modelBuf[512];
                strncpy_s(modelBuf, selected->modelPath.c_str(), sizeof(modelBuf));
                modelBuf[sizeof(modelBuf) - 1] = '\0';
                if (ImGui::InputText("Model Path", modelBuf, sizeof(modelBuf)))
                    selected->modelPath = std::string(modelBuf);

                // Texture path
                char texBuf[512];
                strncpy_s(texBuf, selected->texturePath.c_str(), sizeof(texBuf));
                texBuf[sizeof(texBuf) - 1] = '\0';
                if (ImGui::InputText("Texture Path", texBuf, sizeof(texBuf)))
                    selected->texturePath = std::string(texBuf);

                // Position and rotation
                float pos[3] = { selected->position.x, selected->position.y, selected->position.z };
                if (ImGui::DragFloat3("Position", pos, 0.1f))
                {
                    selected->position = glm::vec3(pos[0], pos[1], pos[2]);
                    if (selected->renderable)
                    {
                        Cube* c = dynamic_cast<Cube*>(selected->renderable);
                        if (c) c->SetModel(selected->position, selected->rotation);
                    }
                }

                float rot[3] = { selected->rotation.x, selected->rotation.y, selected->rotation.z };
                if (ImGui::DragFloat3("Rotation (deg)", rot, 1.0f))
                {
                    selected->rotation = glm::vec3(rot[0], rot[1], rot[2]);
                    if (selected->renderable)
                    {
                        Cube* c = dynamic_cast<Cube*>(selected->renderable);
                        if (c) c->SetModel(selected->position, selected->rotation);
                    }
                }
            }
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

        // set projection and view uniforms
        myShader.Use();
        GLint currentProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        if (static_cast<GLuint>(currentProgram) != myShader.myShaderProgram)
        {
            std::cout << "Current GL program (" << currentProgram << ") is not myShader (" << myShader.myShaderProgram << "). Binding myShader.\n";
            myShader.Use(); // ensure correct program bound before setting uniforms
        }
        else
        {
            // optional: confirm the program is a valid GL program
            if (!glIsProgram(static_cast<GLuint>(currentProgram)))
                std::cerr << "Warning: current program ID is not a valid GL program\n";
        }
        glm::mat4 projection = glm::perspective(glm::radians(g_Camera.GetFov()), (float)display_w / (float)display_h, 0.1f, 100.0f);
        glm::mat4 view = g_Camera.GetViewMatrix();
        // model matrix for the cube (identity -> cube at origin)
        glm::mat4 model = glm::mat4(1.0f);
        // Optionally position/scale/rotate the cube:
        //model = glm::translate(model, glm::vec3(0.0f, 0.0f, 0.0f));
        //model = glm::rotate(model, glm::radians(30.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        // Assumes Shader class exposes a SetMat4(name, glm::mat4) method.
        myShader.SetMat4("projection", projection);
        myShader.SetMat4("view", view);
        myShader.SetMat4("model", model);

        // draw scene
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