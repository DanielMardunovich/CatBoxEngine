#pragma once
#include "../graphics/Shader.h"
#include "../graphics/RenderPipeline.h"
#include "../resources/Entity.h"
#include "../resources/Camera.h"
#include "../resources/EntityManager.h"
#include "../resources/SceneManager.h"
#include "../gameplay/PlayerController.h"
#include "../gameplay/TeleporterSystem.h"
#include "../gameplay/GoalSystem.h"
#include "../gameplay/RecordTimeSystem.h"
#include "UIManager.h"
#include "InputHandler.h"
#include <vector>
#include <string>
#include "../resources/Math/Vec3.h"
#include "Platform.h"

struct GLFWwindow;

class Engine
{
public:
    Engine(float windowWidth = 800.0f, float windowHeight = 800.0f, const char* name = "Catbox Engine");
    ~Engine();

    // Disable copy, enable move
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;
    Engine(Engine&&) noexcept = default;
    Engine& operator=(Engine&&) noexcept = default;

    void app();
    void OnMouseMove(double xpos, double ypos);
    void OnMouseButton(GLFWwindow* window, int button, int action, int mods);
    void OnDrop(const std::vector<std::string>& paths);
    
private:
    void Update(float deltaTime);
    void Render();

    int Initialize();
    int InitImGui();
    void SetupMessageSubscriptions();
    void Cleanup();
    void EnterPlayMode();
    void ExitPlayMode();

    [[nodiscard]] GLFWwindow* GetWindow() const noexcept { return m_window; }
    
    // Window and platform
    GLFWwindow* m_window = nullptr;
    Platform m_platform;

    float m_width;
    float m_height;
    std::string m_name;

    bool m_glfwInitialized = false;
    bool m_imguiInitialized = false;
    
    // Rendering system
    RenderPipeline m_renderPipeline;
    
    // Game systems
    EntityManager m_entityManager;
    UIManager m_uiManager;
    Camera m_camera;
    InputHandler m_inputHandler;
    PlayerController m_playerController;
    TeleporterSystem m_teleporterSystem;
    GoalSystem       m_goalSystem;
    RecordTimeSystem m_recordSystem;

    // Play mode
    bool m_isPlayMode        = false;
    bool m_escapeWasPressed  = false;
    bool m_goalTimeRecorded  = false;   // set true the frame goal is first reached
    float m_completionTime   = -1.0f;   // time submitted for the last completed run
    bool  m_isNewBest        = false;   // whether that time beat the previous record

    // Saved editor camera state (restored when leaving play mode)
    Vec3 m_editorCamPosition;
    Vec3 m_editorCamFront;
    Vec3 m_editorCamUp;
    float m_editorCamYaw = -90.0f;
    float m_editorCamPitch = 0.0f;
    
    // UI state
    Vec3 m_spawnPosition { 0.0f, 0.0f, 0.0f };
    Vec3 m_spawnScale { 0.5f, 0.5f, 0.5f };
    int m_selectedEntityIndex = -1;
    bool m_useSharedCube = true;
};
