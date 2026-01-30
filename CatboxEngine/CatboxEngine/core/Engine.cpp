#include "Engine.h"
#include "Platform.h"
#include "MessageQueue.h"
#include "MemoryTracker.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>
#include <glad/glad.h>
#include <glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "Time.h"
#include "../graphics/LightManager.h"
#include "../resources/SceneManager.h"

Engine::Engine(float windowWidth, float windowHeight, const char* name)
    : m_window(nullptr)
    , m_width(windowWidth)
    , m_height(windowHeight)
    , m_name(name)
    , m_glfwInitialized(false)
    , m_imguiInitialized(false)
{
}

void Engine::OnMouseMove(double xpos, double ypos)
{
    m_camera.OnMouseMove(xpos, ypos);
}

void Engine::OnDrop(const std::vector<std::string>& paths)
{
    m_inputHandler.HandleFileDrop(paths, m_entityManager, m_selectedEntityIndex, m_useSharedCube);
}

void Engine::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    
}

Engine::~Engine()
{
    // Auto-save active scene before shutdown
    auto& sceneMgr = SceneManager::Instance();
    auto* activeScene = sceneMgr.GetActiveScene();
    if (activeScene)
    {
        std::cout << "Auto-saving active scene: " << activeScene->GetName() << std::endl;
        sceneMgr.SaveScene(sceneMgr.GetActiveSceneID(), "autosave.scene", m_entityManager);
    }
    
    Cleanup();
    
    // Check for memory leaks at shutdown
    std::cout << "\n=== Engine Shutdown ===" << std::endl;
    MemoryTracker::Instance().CheckForLeaks();
}

// Forward declare a static callback that will be set on the GLFW window
static void MouseCallback(GLFWwindow* window, double xpos, double ypos)
{
    Engine* eng = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (!eng) return;
    // If ImGui is initialized and wants the mouse, don't forward
    if (ImGui::GetCurrentContext())
    {
        ImGuiIO& io = ImGui::GetIO();
        if (io.WantCaptureMouse) return;
    }
    eng->OnMouseMove(xpos, ypos);
}

static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    Engine* eng = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
    if (!eng) return;
    if (ImGui::GetCurrentContext())
    {
        ImGuiIO& io = ImGui::GetIO();
        bool overUI = ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow) || ImGui::IsAnyItemHovered();
        if (io.WantCaptureMouse && overUI) return;
    }
    eng->OnMouseButton(window, button, action, mods);
}

void Engine::app()
{
    MEMORY_SCOPE("Engine::app");
    
    Initialize();
    
    std::cout << "Initial memory state:" << std::endl;
    MemoryTracker::Instance().PrintMemoryReport();
    
    while (!glfwWindowShouldClose(m_window))
    {
        Time::Update();

        Update(Time::DeltaTime());
        Render();
    }
    
    std::cout << "Final memory state:" << std::endl;
    MemoryTracker::Instance().PrintMemoryReport();
}

void Engine::Update(float deltaTime)
{
    GLFWwindow* window = GetWindow();
    
    // Handle window-level input
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }

    // Let camera handle inputs
    m_camera.Update(window, deltaTime);
    glfwPollEvents();

    // UI frame and draw
    m_uiManager.NewFrame();
    m_uiManager.Draw(m_entityManager, m_spawnPosition, m_spawnScale, deltaTime, m_selectedEntityIndex, m_camera, m_useSharedCube);

    // Poll mesh manager for completed async loads and invoke callbacks
    MeshManager::Instance().PollCompleted();
    
    // Process message queue
    MessageQueue::Instance().ProcessMessages();

    // Check clipboard for dropped path
    if (const char* clip = glfwGetClipboardString(window))
    {
        static std::string lastClip;
        std::string clipStr(clip);
        if (clipStr != lastClip)
        {
            lastClip = clipStr;
            OnDrop({clipStr});
        }
    }
}

void Engine::Render()
{
    GLFWwindow* window = GetWindow();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    
    // Clear screen
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.4f, 0.3f, 0.2f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Render ImGui first (so it's on top)
    ImGui::Render();
    
    // Render scene using RenderPipeline
    m_renderPipeline.Render(m_entityManager, m_camera, display_w, display_h);
    
    // Render UI
    m_uiManager.Render();

    glfwSwapBuffers(window);
}

int Engine::Initialize()
{
    // platform (GLFW window)
    if (!m_platform.Init((int)m_width, (int)m_height, m_name.c_str()))
        return -1;

    m_window = m_platform.GetWindow();

    // Set user pointer so callbacks can access the Engine instance
    glfwSetWindowUserPointer(m_window, this);
    // install input callbacks
    glfwSetCursorPosCallback(m_window, MouseCallback);
    glfwSetMouseButtonCallback(m_window, MouseButtonCallback);
    // install drop callback for drag-and-drop support
    Platform::InstallDropCallback(m_window);

    // glad
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
        return -1;

    glEnable(GL_DEPTH_TEST);

    // Initialize rendering pipeline
    if (!m_renderPipeline.Initialize())
    {
        std::cerr << "Failed to initialize render pipeline!" << std::endl;
        return -1;
    }

    // Initialize camera
    m_camera.Initialize({0,0,3}, {0,0,0}, {0,1,0}, 60.0f, m_width / m_height, 0.1f, 100.0f);

    // ImGui
    if (InitImGui() != 0)
        return -1;

    // Subscribe to messages
    SetupMessageSubscriptions();
    
    // Initialize lighting system with default lights
    auto& lightMgr = LightManager::Instance();
    if (lightMgr.GetLightCount() == 0)
    {
        lightMgr.CreateDefaultLights();
        std::cout << "Default lights initialized" << std::endl;
    }
    
    // Try to load autosave scene
    auto& sceneMgr = SceneManager::Instance();
    std::ifstream checkFile("autosave.scene");
    if (checkFile.good())
    {
        checkFile.close();
        std::cout << "Loading autosave scene..." << std::endl;
        SceneID id = sceneMgr.LoadScene("autosave.scene");
        if (id != 0)
        {
            sceneMgr.SetActiveScene(id, m_entityManager);
        }
    }
    else
    {
        // No autosave, create default scene
        std::cout << "No autosave found, creating default scene..." << std::endl;
        SceneID defaultScene = sceneMgr.CreateScene("Default Scene");
        sceneMgr.SetActiveScene(defaultScene, m_entityManager);
    }

    return 0;
}

void Engine::SetupMessageSubscriptions()
{
    // Subscribe to entity events
    MessageQueue::Instance().Subscribe(MessageType::EntityCreated, [](const Message& msg) {
        const auto& m = static_cast<const EntityCreatedMessage&>(msg);
        std::cout << "Entity created: " << m.entityName << " at index " << m.entityIndex << std::endl;
    });

    MessageQueue::Instance().Subscribe(MessageType::EntityDestroyed, [](const Message& msg) {
        const auto& m = static_cast<const EntityDestroyedMessage&>(msg);
        std::cout << "Entity destroyed: " << m.entityName << " at index " << m.entityIndex << std::endl;
    });

    // Subscribe to mesh events
    MessageQueue::Instance().Subscribe(MessageType::MeshLoaded, [](const Message& msg) {
        const auto& m = static_cast<const MeshLoadedMessage&>(msg);
        std::cout << "Mesh loaded: " << m.path << " (handle: " << m.handle << ")" << std::endl;
    });

    MessageQueue::Instance().Subscribe(MessageType::MeshLoadFailed, [](const Message& msg) {
        const auto& m = static_cast<const MeshLoadFailedMessage&>(msg);
        std::cerr << "Mesh load failed: " << m.path << " - " << m.error << std::endl;
    });
}

int Engine::InitImGui()
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    ImGui::StyleColorsDark();

    // Setup platform/renderer bindings
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 440");

    m_imguiInitialized = true;

    return 0;
}

void Engine::Cleanup()
{
    if (m_imguiInitialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();

        m_imguiInitialized = false;
    }

    if (m_glfwInitialized)
    {
        glfwDestroyWindow(m_window);
        glfwTerminate();

        m_glfwInitialized = false;
    }
}
