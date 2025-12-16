#include "InputManager.h"
#include "../Camera.h"
#include "imgui.h"

#include <iostream>

InputManager& InputManager::Get()
{
    static InputManager instance;
    return instance;
}

InputManager::InputManager() = default;
InputManager::~InputManager() = default;

void InputManager::Initialize(GLFWwindow* window, Camera* camera)
{
    if (!window || !camera)
        return;

    m_Window = window;
    m_Camera = camera;

    // Store pointer to this manager so callbacks can retrieve it
    glfwSetWindowUserPointer(window, this);

    // Register GLFW callbacks that forward to this instance, but keep previous callbacks so we can chain them.
    m_PrevCursorPosCallback = glfwSetCursorPosCallback(window, MousePosCallback);
    m_PrevScrollCallback = glfwSetScrollCallback(window, ScrollCallback);
    m_PrevKeyCallback = glfwSetKeyCallback(window, KeyCallback);
    m_PrevMouseButtonCallback = glfwSetMouseButtonCallback(window, MouseButtonCallback);

    // Start with cursor not captured. We'll capture on left-click.
    SetCursorCaptured(false);

    // initialize last mouse pos
    double x, y;
    glfwGetCursorPos(window, &x, &y);
    m_LastX = x;
    m_LastY = y;
    m_FirstMouse = true;
    m_MouseCaptured = (glfwGetInputMode(window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED);

    m_PendingCapture = false;
    m_PendingRelease = false;
}

void InputManager::SetCursorCaptured(bool captured)
{
    if (!m_Window) return;
    if (captured)
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        m_MouseCaptured = true;
        m_FirstMouse = true; // reset so first delta doesn't jump
    }
    else
    {
        glfwSetInputMode(m_Window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        m_MouseCaptured = false;
    }
}

void InputManager::ProcessInput(float deltaTime)
{
    if (!m_Window || !m_Camera)
        return;

    if (glfwGetKey(m_Window, GLFW_KEY_W) == GLFW_PRESS)
        m_Camera->ProcessKeyboard(Camera::FORWARD, deltaTime);
    if (glfwGetKey(m_Window, GLFW_KEY_S) == GLFW_PRESS)
        m_Camera->ProcessKeyboard(Camera::BACKWARD, deltaTime);
    if (glfwGetKey(m_Window, GLFW_KEY_A) == GLFW_PRESS)
        m_Camera->ProcessKeyboard(Camera::LEFT, deltaTime);
    if (glfwGetKey(m_Window, GLFW_KEY_D) == GLFW_PRESS)
        m_Camera->ProcessKeyboard(Camera::RIGHT, deltaTime);

    // Escape to close
    if (glfwGetKey(m_Window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(m_Window, true);
}

void InputManager::FrameUpdate()
{
    // ImGui state is valid after ImGui::NewFrame() — use it here to decide capture.
    ImGuiIO& io = ImGui::GetIO();

    // If ImGui wants the mouse, release capture immediately.
    if (m_MouseCaptured && io.WantCaptureMouse)
    {
        SetCursorCaptured(false);
    }

    // Apply pending capture if ImGui does not want the mouse
    if (m_PendingCapture)
    {
        if (!io.WantCaptureMouse)
            SetCursorCaptured(true);
        m_PendingCapture = false;
    }

    // Apply pending release always
    if (m_PendingRelease)
    {
        SetCursorCaptured(false);
        m_PendingRelease = false;
    }
}

// --- static callback forwards ------------------------------------------------

void InputManager::MousePosCallback(GLFWwindow* window, double xpos, double ypos)
{
    auto* im = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (im) im->OnMouseMove(xpos, ypos);

    // forward to previous callback (ImGui) so UI gets events
    if (im && im->m_PrevCursorPosCallback)
        im->m_PrevCursorPosCallback(window, xpos, ypos);
}

void InputManager::ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    auto* im = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (im) im->OnScroll(static_cast<float>(yoffset));

    if (im && im->m_PrevScrollCallback)
        im->m_PrevScrollCallback(window, xoffset, yoffset);
}

void InputManager::KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
    auto* im = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (im) im->OnKey(key, action);

    if (im && im->m_PrevKeyCallback)
        im->m_PrevKeyCallback(window, key, scancode, action, mods);
}

void InputManager::MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    auto* im = reinterpret_cast<InputManager*>(glfwGetWindowUserPointer(window));
    if (im) im->OnMouseButton(button, action, mods);

    // forward to previous callback so ImGui processes clicks
    if (im && im->m_PrevMouseButtonCallback)
        im->m_PrevMouseButtonCallback(window, button, action, mods);
}

// --- instance handlers ------------------------------------------------------

void InputManager::OnMouseMove(double xpos, double ypos)
{
    // Only process mouse look while captured
    if (!m_Camera || !m_MouseCaptured)
        return;

    if (m_FirstMouse)
    {
        m_LastX = xpos;
        m_LastY = ypos;
        m_FirstMouse = false;
    }

    float xoffset = static_cast<float>(xpos - m_LastX);
    float yoffset = static_cast<float>(m_LastY - ypos); // reversed
    m_LastX = xpos;
    m_LastY = ypos;

    xoffset *= m_MouseSensitivity;
    yoffset *= m_MouseSensitivity;

    m_Camera->ProcessMouseMovement(xoffset, yoffset);
}

void InputManager::OnScroll(double yoffset)
{
    if (!m_Camera)
        return;
    m_Camera->ProcessMouseScroll(static_cast<float>(yoffset));
}

void InputManager::OnKey(int key, int action)
{
    // Example: toggle cursor capture with C key press
    if (key == GLFW_KEY_C && action == GLFW_PRESS && m_Window)
    {
        int mode = glfwGetInputMode(m_Window, GLFW_CURSOR);
        SetCursorCaptured(mode != GLFW_CURSOR_DISABLED);
    }
}

void InputManager::OnMouseButton(int button, int action, int /*mods*/)
{
    if (!m_Window)
        return;

    // Queue requests — they will be applied in FrameUpdate() after ImGui::NewFrame() runs.
    if (button == GLFW_MOUSE_BUTTON_LEFT)
    {
        if (action == GLFW_PRESS)
        {
            m_PendingCapture = true;
        }
        else if (action == GLFW_RELEASE)
        {
            m_PendingRelease = true;
        }
    }
}