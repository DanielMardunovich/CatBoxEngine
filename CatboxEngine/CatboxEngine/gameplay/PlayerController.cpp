#include "PlayerController.h"
#include <glfw3.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <algorithm>
#include <iostream>

PlayerController::PlayerController()
{
}

void PlayerController::Initialize(Entity* playerEntity, Camera* camera)
{
    m_playerEntity = playerEntity;
    m_camera = camera;
    
    if (m_playerEntity)
    {
        // Initialize player yaw to face forward (-Z)
        m_playerYaw = 0.0f;
        m_playerEntity->Transform.Rotation.y = m_playerYaw;
    }
    
    if (m_camera)
    {
        // Set initial camera position
        m_cameraYaw = 0.0f;
        m_cameraPitch = 20.0f;
        UpdateCamera(0.0f);
    }
    
    std::cout << "PlayerController initialized" << std::endl;
}

void PlayerController::Update(GLFWwindow* window, float deltaTime)
{
    if (!m_enabled || !m_playerEntity || !m_camera)
        return;
    
    UpdateMovement(window, deltaTime);
    UpdateCamera(deltaTime);
    UpdatePlayerState();
}

void PlayerController::UpdateMovement(GLFWwindow* window, float deltaTime)
{
    // Get input direction
    glm::vec2 input = GetInputVector(window);
    
    // Check ground collision
    CheckGroundCollision();
    
    // Calculate camera-relative movement direction
    glm::vec3 moveDirection(0.0f);
    
    if (glm::length(input) > 0.01f)
    {
        // Get camera forward and right vectors (projected on XZ plane)
        float yawRad = glm::radians(m_cameraYaw);
        glm::vec3 cameraForward(-sin(yawRad), 0.0f, cos(yawRad));
        glm::vec3 cameraRight(-cos(yawRad), 0.0f, -sin(yawRad));
        
        // Calculate movement direction relative to camera
        moveDirection = glm::normalize(cameraForward * input.y + cameraRight * input.x);
        
        // Update player rotation to face movement direction
        float targetYaw = glm::degrees(atan2(moveDirection.x, -moveDirection.z));
        
        // Smoothly rotate player towards target
        float yawDiff = targetYaw - m_playerYaw;
        
        // Normalize angle difference to [-180, 180]
        while (yawDiff > 180.0f) yawDiff -= 360.0f;
        while (yawDiff < -180.0f) yawDiff += 360.0f;

        float maxTurn = MovementConfig.TurnSpeed * deltaTime;
        float turnAmount = glm::clamp(yawDiff, -maxTurn, maxTurn);
        
        m_playerYaw += turnAmount;
        m_playerEntity->Transform.Rotation.y = m_playerYaw;
    }
    
    // Handle jumping
    if (m_isGrounded && glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        m_velocity.y = MovementConfig.JumpForce;
        m_isGrounded = false;
    }
    
    // Determine target speed
    bool isRunning = glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    float targetSpeed = isRunning ? MovementConfig.RunSpeed : MovementConfig.WalkSpeed;
    
    // Calculate target velocity
    if (glm::length(input) > 0.01f)
    {
        m_targetVelocity = moveDirection * targetSpeed;
    }
    else
    {
        m_targetVelocity = glm::vec3(0.0f);
    }
    
    // Apply acceleration/deceleration
    float accel = (glm::length(input) > 0.01f) ? MovementConfig.Acceleration : MovementConfig.Deceleration;
    
    // Horizontal velocity interpolation
    glm::vec3 horizontalVel(m_velocity.x, 0.0f, m_velocity.z);
    glm::vec3 targetHorizontalVel(m_targetVelocity.x, 0.0f, m_targetVelocity.z);
    
    horizontalVel = glm::mix(horizontalVel, targetHorizontalVel, accel * deltaTime);
    
    m_velocity.x = horizontalVel.x;
    m_velocity.z = horizontalVel.z;
    
    // Apply gravity
    if (!m_isGrounded)
    {
        m_velocity.y -= MovementConfig.Gravity * deltaTime;
    }
    else
    {
        // Keep slight downward velocity when grounded
        m_velocity.y = -0.1f;
    }
    
    // Apply velocity to position
    glm::vec3 displacement = m_velocity * deltaTime;
    m_playerEntity->Transform.Position.x += displacement.x;
    m_playerEntity->Transform.Position.y += displacement.y;
    m_playerEntity->Transform.Position.z += displacement.z;
    
    // Simple ground clamping (replace with proper collision later)
    if (m_playerEntity->Transform.Position.y < 0.0f)
    {
        m_playerEntity->Transform.Position.y = 0.0f;
        m_velocity.y = 0.0f;
        m_isGrounded = true;
    }
}

void PlayerController::UpdateCamera(float deltaTime)
{
    if (!m_playerEntity || !m_camera)
        return;

    glm::vec3 desiredPos = CalculateDesiredCameraPosition();

    // Snap immediately on first frame (deltaTime==0), otherwise smooth follow
    if (deltaTime <= 0.0f)
    {
        m_currentCameraPos = desiredPos;
    }
    else
    {
        m_currentCameraPos = glm::mix(m_currentCameraPos, desiredPos,
                                       CameraConfig.SmoothSpeed * deltaTime);
    }

    m_camera->Position.x = m_currentCameraPos.x;
    m_camera->Position.y = m_currentCameraPos.y;
    m_camera->Position.z = m_currentCameraPos.z;

    // Camera looks at player center
    glm::vec3 playerPos(m_playerEntity->Transform.Position.x,
                       m_playerEntity->Transform.Position.y,
                       m_playerEntity->Transform.Position.z);
    glm::vec3 lookTarget = playerPos + glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 direction = glm::normalize(lookTarget - m_currentCameraPos);

    m_camera->Front.x = direction.x;
    m_camera->Front.y = direction.y;
    m_camera->Front.z = direction.z;
}

void PlayerController::UpdatePlayerState()
{
    float horizontalSpeed = glm::length(glm::vec2(m_velocity.x, m_velocity.z));
    
    if (!m_isGrounded)
    {
        if (m_velocity.y > 0.1f)
            m_state = PlayerState::Jumping;
        else
            m_state = PlayerState::Falling;
    }
    else if (horizontalSpeed > 0.1f)
    {
        if (horizontalSpeed > MovementConfig.WalkSpeed * 1.2f)
            m_state = PlayerState::Running;
        else
            m_state = PlayerState::Walking;
    }
    else
    {
        m_state = PlayerState::Idle;
    }
}

void PlayerController::CheckGroundCollision()
{
    // Simple ground check - player is grounded if on or below y=0
    // You can extend this to raycast down for proper collision
    float groundY = 0.0f;
    float playerY = m_playerEntity->Transform.Position.y;
    
    m_isGrounded = (playerY <= groundY + MovementConfig.GroundCheckDistance);
}

glm::vec2 PlayerController::GetInputVector(GLFWwindow* window)
{
    glm::vec2 input(0.0f);
    
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) input.y += 1.0f;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) input.y -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) input.x -= 1.0f;
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) input.x += 1.0f;
    
    // Normalize diagonal movement
    if (glm::length(input) > 1.0f)
        input = glm::normalize(input);
    
    return input;
}

glm::vec3 PlayerController::CalculateDesiredCameraPosition() const
{
    glm::vec3 playerPos(m_playerEntity->Transform.Position.x,
                       m_playerEntity->Transform.Position.y + 1.0f, // Offset to player center
                       m_playerEntity->Transform.Position.z);
    
    // Calculate camera offset based on yaw and pitch
    float yawRad = glm::radians(m_cameraYaw);
    float pitchRad = glm::radians(m_cameraPitch);
    
    // Calculate camera position in spherical coordinates
    float horizontalDist = CameraConfig.Distance * cos(pitchRad);
    float verticalDist = CameraConfig.Distance * sin(pitchRad);
    
    glm::vec3 offset(
        horizontalDist * sin(yawRad),
        verticalDist + CameraConfig.Height,
        horizontalDist * -cos(yawRad)
    );
    
    return playerPos + offset;
}

float PlayerController::GetCurrentSpeed() const
{
    return glm::length(glm::vec2(m_velocity.x, m_velocity.z));
}

void PlayerController::OnPlayModeEnter()
{
    // Reset velocity so player doesn't start moving
    m_velocity = glm::vec3(0.0f);
    m_targetVelocity = glm::vec3(0.0f);

    // Capture mouse and snap camera to behind player immediately
    m_cursorCaptured = true;
    m_firstMouse = true;
    UpdateCamera(0.0f);
}

void PlayerController::TeleportTo(const Vec3& position)
{
    if (!m_playerEntity)
        return;
    m_playerEntity->Transform.Position = position;
    m_velocity = glm::vec3(0.0f);
    m_targetVelocity = glm::vec3(0.0f);
    // Snap current camera position so it doesn't lerp from the old spot
    m_currentCameraPos = CalculateDesiredCameraPosition();
}

void PlayerController::OnPlayModeExit()
{
    m_cursorCaptured = false;
    m_firstMouse = true;
}

void PlayerController::OnMouseMove(double xpos, double ypos)
{
    if (!m_cursorCaptured || !m_enabled)
        return;
    
    if (m_firstMouse)
    {
        m_lastMouseX = static_cast<float>(xpos);
        m_lastMouseY = static_cast<float>(ypos);
        m_firstMouse = false;
        return;
    }
    
    float xoffset = static_cast<float>(xpos) - m_lastMouseX;
    float yoffset = static_cast<float>(ypos) - m_lastMouseY;
    
    m_lastMouseX = static_cast<float>(xpos);
    m_lastMouseY = static_cast<float>(ypos);
    
    xoffset *= CameraConfig.Sensitivity;
    yoffset *= CameraConfig.Sensitivity;
    
    m_cameraYaw += xoffset;
    m_cameraPitch += yoffset;

    // Constrain pitch
    m_cameraPitch = glm::clamp(m_cameraPitch, CameraConfig.MinPitch, CameraConfig.MaxPitch);
    
    // Normalize yaw to [0, 360)
    while (m_cameraYaw >= 360.0f) m_cameraYaw -= 360.0f;
    while (m_cameraYaw < 0.0f) m_cameraYaw += 360.0f;
}

void PlayerController::OnMouseButton(int button, int action)
{
    if (!m_enabled)
        return;
    
    if (button == GLFW_MOUSE_BUTTON_RIGHT)
    {
        if (action == GLFW_PRESS)
        {
            m_cursorCaptured = true;
            m_firstMouse = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_cursorCaptured = false;
        }
    }
}
