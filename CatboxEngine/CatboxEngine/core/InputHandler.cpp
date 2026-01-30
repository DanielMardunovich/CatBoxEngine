#include "InputHandler.h"
#include "MessageQueue.h"
#include "../graphics/Mesh.h"
#include "../graphics/MeshManager.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include <algorithm>
#include <cctype>

void InputHandler::HandleFileDrop(const std::vector<std::string>& paths, EntityManager& entityManager,
                                  int selectedEntityIndex, bool useSharedCube)
{
    if (paths.empty())
    {
        return;
    }

    // Handle first dropped file
    const std::string& path = paths[0];
    const std::string ext = GetFileExtension(path);

    if (ext == "obj" || ext == "gltf" || ext == "glb")
    {
        HandleModelDrop(path, entityManager, useSharedCube);
    }
    else if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
    {
        HandleTextureDrop(path, entityManager, selectedEntityIndex);
    }
}

void InputHandler::HandleModelDrop(const std::string& path, EntityManager& entityManager, bool useSharedCube)
{
    // Post model dropped message
    auto msg = std::make_shared<ModelDroppedMessage>(path);
    MessageQueue::Instance().Post(msg);

    MeshHandle handle = MeshManager::Instance().LoadMeshSync(path);
    if (handle != 0)
    {
        Entity entity;
        entity.name = "Model: " + path;
        entity.MeshHandle = handle;
        entityManager.AddEntity(entity, useSharedCube);
    }
}

void InputHandler::HandleTextureDrop(const std::string& path, EntityManager& entityManager, int selectedEntityIndex)
{
    // Post texture dropped message
    auto msg = std::make_shared<TextureDroppedMessage>(path);
    MessageQueue::Instance().Post(msg);

    // Assign texture to selected entity if valid
    if (selectedEntityIndex >= 0 && selectedEntityIndex < static_cast<int>(entityManager.Size()))
    {
        auto& entity = entityManager.GetAll()[selectedEntityIndex];
        if (entity.MeshHandle != 0)
        {
            Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
            if (mesh)
            {
                mesh->LoadTexture(path);
                mesh->DiffuseTexturePath = path;

                // Post texture loaded message
                auto loadMsg = std::make_shared<TextureLoadedMessage>(path, selectedEntityIndex);
                MessageQueue::Instance().Post(loadMsg);
            }
        }
    }
}

std::string InputHandler::GetFileExtension(const std::string& path)
{
    const auto pos = path.find_last_of('.');
    if (pos == std::string::npos)
    {
        return "";
    }

    std::string ext = path.substr(pos + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}
