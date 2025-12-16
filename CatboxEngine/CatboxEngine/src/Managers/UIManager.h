#pragma once
// Forward-declare GLFW types to avoid pulling OpenGL headers into this header.
// Include <glad/glad.h> before any system OpenGL headers in .cpp files.
struct GLFWwindow;


struct Entity;

class UIManager
{
public:
    static UIManager& Get();

    // Initialize ImGui with the given GLFW window
    bool Initialize(GLFWwindow* window, const char* glsl_version = "#version 440");

    // NewFrame must be called at the start of each frame (it calls ImGui_Impl* NewFrame + ImGui::NewFrame)
    void NewFrame();

    // Renders UI to the screen (calls ImGui::Render and the OpenGL backend)
    void Render();

    // Clean up ImGui and backends
    void Shutdown();

private:
    UIManager();
    ~UIManager();

    // UI windows
    void DrawEntitiesWindow();

    GLFWwindow* m_Window = nullptr;
    const char* m_GlslVersion = nullptr;

    // selection state for entities UI
    struct Entity* m_SelectedEntity = nullptr;

    bool m_Initialized = false;
};