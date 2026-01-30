#pragma once

#include <vector>
#include <string>

struct GLFWwindow;
class EntityManager;
class Camera;

class InputHandler
{
public:
    InputHandler() = default;
    ~InputHandler() = default;

    // Disable copy, enable move
    InputHandler(const InputHandler&) = delete;
    InputHandler& operator=(const InputHandler&) = delete;
    InputHandler(InputHandler&&) noexcept = default;
    InputHandler& operator=(InputHandler&&) noexcept = default;

    // Mouse input
    void HandleMouseMove(double xpos, double ypos, Camera& camera);
    void HandleMouseButton(GLFWwindow* window, int button, int action, int mods, Camera& camera);

    // File drop events (drag and drop)
    void HandleFileDrop(const std::vector<std::string>& paths, EntityManager& entityManager, 
                       int selectedEntityIndex, bool useSharedCube);

    // Clipboard monitoring for file drops
    void CheckClipboardForDrop(GLFWwindow* window, EntityManager& entityManager, 
                              int selectedEntityIndex, bool useSharedCube);

    // Keyboard input
    bool ShouldCloseWindow(GLFWwindow* window) const;

private:
    void HandleModelDrop(const std::string& path, EntityManager& entityManager, bool useSharedCube);
    void HandleTextureDrop(const std::string& path, EntityManager& entityManager, int selectedEntityIndex);
    
    static std::string GetFileExtension(const std::string& path);

    // Clipboard state for drop detection
    std::string m_lastClipboard;
};
