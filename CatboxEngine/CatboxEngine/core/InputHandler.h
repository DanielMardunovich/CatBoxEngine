#pragma once

#include <vector>
#include <string>

class EntityManager;

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

    // Handle file drop events (drag and drop)
    void HandleFileDrop(const std::vector<std::string>& paths, EntityManager& entityManager, 
                       int selectedEntityIndex, bool useSharedCube);

private:
    void HandleModelDrop(const std::string& path, EntityManager& entityManager, bool useSharedCube);
    void HandleTextureDrop(const std::string& path, EntityManager& entityManager, int selectedEntityIndex);
    
    static std::string GetFileExtension(const std::string& path);
};
