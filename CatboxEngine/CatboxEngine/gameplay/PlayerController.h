#pragma once
#include "../resources/Entity.h"
#include "../resources/Camera.h"
#include "AnimationSystem.h"
#include <glm/glm.hpp>
#include <unordered_map>
#include <memory>

struct GLFWwindow;
class EntityManager;

// Player movement state
enum class PlayerState
{
    Idle,
    Walking,
    Running,
    Jumping,
    Falling,
    Landing
};

// Third person camera controller for Mario 64-style platformer
class PlayerController
{
public:
    PlayerController();
    
    // Initialize with player entity and camera
    void Initialize(Entity* playerEntity, Camera* camera);
    
    // Update player movement and camera
    void Update(GLFWwindow* window, float deltaTime, EntityManager& entityManager);
    
    // Camera settings
    struct CameraSettings
    {
        float Distance = 8.0f;           // Distance behind player
        float Height = 3.0f;             // Height above player
        float Sensitivity = 0.15f;       // Mouse sensitivity
        float MinPitch = -30.0f;         // Min camera pitch (degrees)
        float MaxPitch = 60.0f;          // Max camera pitch (degrees)
        float SmoothSpeed = 10.0f;       // Camera follow smoothing
        float CollisionRadius = 0.3f;    // Camera collision sphere radius
    } CameraConfig;
    
    // Movement settings
    struct MovementSettings
    {
        float WalkSpeed = 5.0f;
        float RunSpeed = 10.0f;
        float Acceleration = 20.0f;
        float Deceleration = 15.0f;
        float TurnSpeed = 720.0f;        // Degrees per second
        float JumpForce = 8.0f;
        float Gravity = 20.0f;
        float GroundCheckDistance = 0.1f;
    } MovementConfig;
    
    // Input handling
    void OnMouseMove(double xpos, double ypos); 
    void OnMouseButton(int button, int action);

    // Play mode transitions
    void OnPlayModeEnter();
    void OnPlayModeExit();
    // Instantly move player to position and reset velocity
    void TeleportTo(const Vec3& position);

    // Getters
    [[nodiscard]] PlayerState GetState() const { return m_state; }
    [[nodiscard]] bool IsGrounded() const { return m_isGrounded; }
    [[nodiscard]] glm::vec3 GetVelocity() const { return m_velocity; }
    [[nodiscard]] float GetCameraYaw() const { return m_cameraYaw; }
    [[nodiscard]] float GetCameraPitch() const { return m_cameraPitch; }
    [[nodiscard]] bool HasPlayerEntity() const { return m_playerEntity != nullptr; }
    [[nodiscard]] Vec3 GetPlayerPosition() const
    {
        if (!m_playerEntity) return Vec3(0.0f, 0.0f, 0.0f);
        return m_playerEntity->Transform.Position;
    }

    // Enable/disable controller
    void SetEnabled(bool enabled) { m_enabled = enabled; }
    [[nodiscard]] bool IsEnabled() const { return m_enabled; }

    // Animation: load clips from the entity's assigned FBX paths
    void LoadAnimations();
    // Get the animation player (for debug display)
    [[nodiscard]] const AnimationPlayer& GetAnimationPlayer() const { return m_animPlayer; }

private:
    // Core references
    Entity* m_playerEntity = nullptr;
    Camera* m_camera = nullptr;
    
    // Player state
    PlayerState m_state = PlayerState::Idle;
    glm::vec3 m_velocity { 0.0f };
    glm::vec3 m_targetVelocity { 0.0f };
    bool m_isGrounded = false;
    float m_playerYaw = 0.0f;            // Player facing direction
    
    // Camera state
    float m_cameraYaw = 0.0f;            // Camera horizontal rotation
    float m_cameraPitch = 20.0f;         // Camera vertical rotation
    glm::vec3 m_currentCameraPos { 0.0f };
    
    // Mouse input state
    float m_lastMouseX = 0.0f;
    float m_lastMouseY = 0.0f;
    bool m_firstMouse = true;
    bool m_cursorCaptured = false;
    
    // Control state
    bool m_enabled = true;
    
    // Internal methods
    void UpdateMovement(GLFWwindow* window, float deltaTime, EntityManager& entityManager);
    void UpdateCamera(float deltaTime);
    void UpdatePlayerState();
    void UpdateAnimation(float deltaTime);
    glm::vec2 GetInputVector(GLFWwindow* window);
    glm::vec3 CalculateDesiredCameraPosition() const;
    float GetCurrentSpeed() const;

    // Animation
    AnimationPlayer m_animPlayer;
    std::unordered_map<PlayerState, std::unique_ptr<AnimationClip>> m_clips;
    PlayerState m_prevAnimState = PlayerState::Idle;
};
