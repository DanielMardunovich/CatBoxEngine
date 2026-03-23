#include "Engine.h"
#include "Platform.h"
#include "MessageQueue.h"
#include "MemoryTracker.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <iostream>
#include <fstream>
#include <direct.h>  // For _mkdir
#include <glad/glad.h>
#include <glfw3.h>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>
#include "Time.h"
#include "../graphics/MeshManager.h"
#include "../graphics/LightManager.h"
#include "../resources/SceneManager.h"

static constexpr const char* k_autosaveDir  = "F:\\EngineSpecialization\\CatBoxEngine\\CatboxEngine\\CatboxEngine\\Scenes";
static constexpr const char* k_autosavePath = "F:\\EngineSpecialization\\CatBoxEngine\\CatboxEngine\\CatboxEngine\\Scenes\\autosave.scene";

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
    if (m_isPlayMode)
    {
        m_playerController.OnMouseMove(xpos, ypos);
    }
    else
    {
        m_inputHandler.HandleMouseMove(xpos, ypos, m_camera);
    }
}

void Engine::OnDrop(const std::vector<std::string>& paths)
{
    m_inputHandler.HandleFileDrop(paths, m_entityManager, m_selectedEntityIndex, m_useSharedCube);
}

void Engine::OnMouseButton(GLFWwindow* window, int button, int action, int mods)
{
    if (m_isPlayMode)
    {
        // In play mode the cursor is always locked; ignore button-driven cursor toggling
        return;
    }
    m_inputHandler.HandleMouseButton(window, button, action, mods, m_camera);
}

Engine::~Engine()
{
    // Auto-save active scene before shutdown
    auto& sceneMgr = SceneManager::Instance();
    auto* activeScene = sceneMgr.GetActiveScene();
    if (activeScene)
    {
        _mkdir(k_autosaveDir);  // Create directory if it doesn't exist (fails silently if already present)
        std::cout << "Auto-saving active scene: " << activeScene->GetName()
                  << " to " << k_autosavePath << std::endl;
        sceneMgr.SaveScene(sceneMgr.GetActiveSceneID(), k_autosavePath, m_entityManager);
    }
    
    Cleanup();
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

    // Single debounced ESC handler — behaviour depends on current mode
    bool escDown = glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    if (escDown && !m_escapeWasPressed)
    {
        if (m_isPlayMode)
        {
            // Exit play mode; do NOT close the window
            m_isPlayMode = false;
            ExitPlayMode();
        }
        else
        {
            glfwSetWindowShouldClose(window, true);
        }
    }
    m_escapeWasPressed = escDown;

    // Update player or free camera
    if (m_isPlayMode)
    {
        m_recordSystem.Update(deltaTime);

        // Freeze player input once the goal is reached
        if (!m_goalSystem.IsGoalReached())
        {
            m_playerController.Update(window, deltaTime, m_entityManager);
            m_teleporterSystem.Update(m_entityManager, m_playerController, deltaTime);
            m_enemySystem.Update(m_entityManager, m_playerController, deltaTime);
        }
        m_goalSystem.Update(m_entityManager, m_playerController);

        // On the first frame the goal is reached, stop the timer and record the time
        if (m_goalSystem.IsGoalReached() && !m_goalTimeRecorded)
        {
            m_recordSystem.Stop();
            m_completionTime = m_recordSystem.GetCurrentTime();

            auto* scene = SceneManager::Instance().GetActiveScene();
            if (scene && !scene->GetFilePath().empty())
                m_isNewBest = m_recordSystem.SubmitTime(scene->GetFilePath(), m_completionTime);
            else
                m_isNewBest = false;

            m_uiManager.NotifyGoalResult(m_completionTime, m_isNewBest);
            m_goalTimeRecorded = true;
        }
    }
    else
    {
        m_camera.Update(window, deltaTime);
    }

    glfwPollEvents();

    // UI frame and draw
    bool prevPlayMode = m_isPlayMode;
    m_uiManager.NewFrame();
    m_uiManager.Draw(m_entityManager, m_spawnPosition, m_spawnScale, deltaTime,
                     m_selectedEntityIndex, m_camera, m_useSharedCube,
                     &m_playerController, m_isPlayMode, m_goalSystem.IsGoalReached(),
                     &m_recordSystem);

    // React to play mode toggle from the Stop/Play toolbar button
    if (!prevPlayMode && m_isPlayMode)
        EnterPlayMode();
    else if (prevPlayMode && !m_isPlayMode)
        ExitPlayMode();

    // Detect scene changes (level loads happen inside m_uiManager.Draw above).
    // When the active scene ID changes, find the entity flagged IsPlayer and
    // re-initialize the controller so the pointer is always valid.
    {
        SceneID currentSceneID = SceneManager::Instance().GetActiveSceneID();
        if (currentSceneID != m_lastSceneID)
        {
            m_lastSceneID = currentSceneID;
            Entity* playerEntity = m_entityManager.FindPlayerEntity();
            if (playerEntity)
                m_playerController.Initialize(playerEntity, &m_camera);
        }
    }

    // Poll mesh manager for completed async loads and invoke callbacks
    MeshManager::Instance().PollCompleted();

    // Process message queue
    MessageQueue::Instance().ProcessMessages();

    // Check clipboard for dropped path
    m_inputHandler.CheckClipboardForDrop(window, m_entityManager, m_selectedEntityIndex, m_useSharedCube);
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

    // Draw patrol waypoint overlay only while in the editor
    if (!m_isPlayMode)
        m_renderPipeline.RenderWaypointOverlay(m_entityManager, m_camera, display_w, display_h);

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
    m_camera.Initialize({0,0,3}, {0,0,0}, {0,1,0}, 60.0f, m_width / m_height, 0.1f, 100.0f, 2.5f);

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
    std::ifstream checkFile(k_autosavePath);
    if (checkFile.good())
    {
        checkFile.close();
        std::cout << "Loading autosave scene..." << std::endl;
        SceneID id = sceneMgr.LoadScene(k_autosavePath);
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

void Engine::EnterPlayMode()
{
    // Save editor camera state so we can restore it on exit
    m_editorCamPosition = m_camera.Position;
    m_editorCamFront    = m_camera.Front;
    m_editorCamUp       = m_camera.Up;
    m_editorCamYaw      = m_camera.Yaw;
    m_editorCamPitch    = m_camera.Pitch;

    // Re-resolve the player entity in case the scene was reloaded since last assignment
    if (!m_playerController.HasPlayerEntity())
    {
        Entity* playerEntity = m_entityManager.FindPlayerEntity();
        if (playerEntity)
            m_playerController.Initialize(playerEntity, &m_camera);
    }

    // Teleport player to spawn point before the camera snaps
    Entity* spawnPoint = m_entityManager.FindSpawnPoint();
    if (spawnPoint && m_playerController.HasPlayerEntity())
    {
        m_playerController.TeleportTo(spawnPoint->Transform.Position);
        std::cout << "Player teleported to spawn point: "
                  << spawnPoint->name << std::endl;
    }

    // Lock cursor and hand off to player controller
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    m_playerController.OnPlayModeEnter();
    m_enemySystem.EnterPlayMode(m_entityManager);

    // Start the run timer
    m_recordSystem.Start();
    m_goalTimeRecorded = false;
    m_completionTime   = -1.0f;
    m_isNewBest        = false;

    std::cout << "Entered Play Mode" << std::endl;
}

void Engine::ExitPlayMode()
{
    // Release cursor
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    m_playerController.OnPlayModeExit();
    m_teleporterSystem.Reset();
    m_goalSystem.Reset();
    m_enemySystem.ExitPlayMode(m_entityManager);
    m_recordSystem.Stop();

    // Restore editor camera
    m_camera.Position = m_editorCamPosition;
    m_camera.Front    = m_editorCamFront;
    m_camera.Up       = m_editorCamUp;
    m_camera.Yaw      = m_editorCamYaw;
    m_camera.Pitch    = m_editorCamPitch;

    std::cout << "Exited Play Mode" << std::endl;
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

    // Always save imgui.ini in the Scenes folder regardless of working directory
    static std::string s_iniPath = std::string(k_autosaveDir) + "\\imgui.ini";
    io.IniFilename = s_iniPath.c_str();

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
