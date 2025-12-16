#pragma once
#include <glfw3.h>

class Camera;

/// Small input manager to centralize keyboard / mouse handling.
/// Usage:
///   InputManager::Get().Initialize(window, cameraPtr);
///   In your main loop call: InputManager::Get().ProcessInput(deltaTime);
class InputManager
{
public:
    static InputManager& Get();

    // Initialize with the GLFW window and a pointer to the camera to control.
    // This registers GLFW callbacks and stores the camera pointer.
    void Initialize(GLFWwindow* window, Camera* camera);

    // Called each frame to handle keyboard-based movement using deltaTime.
    void ProcessInput(float deltaTime);

    // Called once per frame after ImGui::NewFrame() to apply pending mouse/button decisions.
    void FrameUpdate();

    // Toggle cursor mode (true = disabled / captured)
    void SetCursorCaptured(bool captured);

private:
    InputManager();
    ~InputManager();

    // GLFW callback forwards
    static void MousePosCallback(GLFWwindow* window, double xpos, double ypos);
    static void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);
    static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods);
    static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);

    // instance methods called by callbacks
    void OnMouseMove(double xpos, double ypos);
    void OnScroll(double yoffset);
    void OnKey(int key, int action);
    void OnMouseButton(int button, int action, int mods);

    GLFWwindow* m_Window = nullptr;
    Camera* m_Camera = nullptr;

    bool m_FirstMouse = true;
    double m_LastX = 0.0;
    double m_LastY = 0.0;

    float m_MouseSensitivity = 0.1f;
    float m_MovementSpeed = 2.5f;

    bool m_MouseCaptured = false;

    // pending actions set by GLFW callbacks and applied in FrameUpdate()
    bool m_PendingCapture = false;
    bool m_PendingRelease = false;

    // previously installed callbacks (so we can forward to ImGui's callbacks)
    GLFWcursorposfun       m_PrevCursorPosCallback = nullptr;
    GLFWscrollfun          m_PrevScrollCallback = nullptr;
    GLFWkeyfun             m_PrevKeyCallback = nullptr;
    GLFWmousebuttonfun     m_PrevMouseButtonCallback = nullptr;
};