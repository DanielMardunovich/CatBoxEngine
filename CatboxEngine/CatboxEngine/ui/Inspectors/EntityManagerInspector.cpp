#include "EntityManagerInspector.h"
#include "../../core/Platform.h"
#include "../../resources/EntityManager.h"
#include "../../resources/Entity.h"
#include "../../graphics/MeshManager.h"
#include "../../graphics/Mesh.h"
#include "../../graphics/GraphicsSettings.h"
#include "../../gameplay/TerrainSystem.h"
#include "../../Dependencies/stb_image.h"
#include "imgui.h"
#include <glad/glad.h>
#include <algorithm>
#include <cctype>
#include <iostream>

// Constants
namespace
{
    constexpr int ENTITY_LIST_HEIGHT = 200;
    constexpr float DELETE_BUTTON_WIDTH = 90.0f;
    constexpr int DIFFUSE_CHANNELS = 4;
    constexpr int SPECULAR_CHANNELS = 1;
    constexpr int NORMAL_CHANNELS = 4;
}

void EntityManagerInspector::Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale,
                                 int& selectedIndex, bool& useSharedCube)
{
    ImGui::Begin("Entity Inspector");

    ImGui::Text("Entities: %zu", entityManager.Size());
    ImGui::Separator();

    DrawSpawnControls(entityManager, spawnPosition, spawnScale, selectedIndex, useSharedCube);
    ImGui::Separator();

    ImGui::Text("Entity List:");
    DrawEntityList(entityManager, selectedIndex);
    ImGui::Separator();

    if (selectedIndex >= 0 && selectedIndex < static_cast<int>(entityManager.Size()))
    {
        DrawFullEntityInspector(entityManager, selectedIndex);
    }

    ImGui::End();

    DrawTextureAssignmentPopup(entityManager, selectedIndex);
    DrawModelErrorPopup();
}

void EntityManagerInspector::DrawSpawnControls(EntityManager& entityManager, Vec3& spawnPosition,
                                              Vec3& spawnScale, int selectedIndex, bool useSharedCube)
{
    // Spawn position
    ImGui::Text("Spawn Position");
    ImGui::DragFloat3("##SpawnPos", &spawnPosition.x, 0.1f);

    // Scale
    ImGui::Text("Scale");
    ImGui::DragFloat3("##SpawnScale", &spawnScale.x, 0.01f, 0.001f, 100.0f);

    // Model path
    ImGui::Text("Model Path");
    ImGui::InputText("##ModelPath", m_modelPath, IM_ARRAYSIZE(m_modelPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        char szFile[260] = {0};
        if (Platform::OpenFileDialog(szFile, sizeof(szFile),
            "3D Models\0*.obj;*.gltf;*.glb;*.fbx\0OBJ Files\0*.obj\0GLTF Files\0*.gltf;*.glb\0FBX Files\0*.fbx\0All\0*.*\0"))
        {
            strncpy_s(m_modelPath, szFile, sizeof(m_modelPath));
        }
    }

    // Spawn buttons
    if (ImGui::Button("Spawn Entity"))
    {
        SpawnNewEntity(entityManager, spawnPosition, spawnScale, useSharedCube);
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply to Selected"))
    {
        ApplyModelToSelected(entityManager, selectedIndex);
    }
    ImGui::SameLine();
    if (ImGui::Button("Create Spawn Point"))
    {
        Entity sp;
        sp.name = "Spawn Point";
        sp.Transform.Position = spawnPosition;
        sp.Transform.Scale = Vec3(0.5f, 0.5f, 0.5f);
        sp.IsSpawnPoint = true;
        entityManager.AddEntity(sp, true);
    }
    ImGui::SetItemTooltip("Creates a marker entity. The player will spawn here when Play Mode starts.");

    if (ImGui::Button("Spawn Teleporter Pair"))
    {
        int pairID = entityManager.GetNextTeleporterPairID();
        std::string pairLabel = std::to_string(pairID);

        Entity tpA;
        tpA.name = "Teleporter A (Pair " + pairLabel + ")";
        tpA.Transform.Position = spawnPosition;
        tpA.Transform.Scale = Vec3(1.0f, 1.5f, 1.0f);
        tpA.IsTeleporter = true;
        tpA.TeleporterPairID = pairID;
        tpA.CollidesWithPlayer = false;
        entityManager.AddEntity(tpA, true);

        Entity tpB;
        tpB.name = "Teleporter B (Pair " + pairLabel + ")";
        tpB.Transform.Position = Vec3(spawnPosition.x + 8.0f, spawnPosition.y, spawnPosition.z);
        tpB.Transform.Scale = Vec3(1.0f, 1.5f, 1.0f);
        tpB.IsTeleporter = true;
        tpB.TeleporterPairID = pairID;
        tpB.CollidesWithPlayer = false;
        entityManager.AddEntity(tpB, true);
    }
    ImGui::SetItemTooltip("Spawns two linked teleporters. Move them apart, then enter Play Mode to test.");

    if (ImGui::Button("Spawn Goal"))
    {
        Entity goal;
        goal.name = "Goal";
        goal.Transform.Position = spawnPosition;
        goal.Transform.Scale = Vec3(1.0f, 2.0f, 1.0f);
        goal.IsGoal = true;
        goal.CollidesWithPlayer = false;
        entityManager.AddEntity(goal, true);
    }
    ImGui::SetItemTooltip("Spawns a goal entity. When the player reaches it in Play Mode, the level is complete.");

    if (ImGui::Button("Spawn Enemy"))
    {
        Entity enemy;
        enemy.name = "Enemy";
        enemy.Transform.Position = spawnPosition;
        enemy.Transform.Scale = Vec3(0.8f, 1.0f, 0.8f);
        enemy.IsEnemy = true;
        enemy.CollidesWithPlayer = false;
        enemy.PatrolWaypoints.push_back(spawnPosition);
        enemy.PatrolWaypoints.push_back(Vec3(spawnPosition.x + 4.0f, spawnPosition.y, spawnPosition.z));
        entityManager.AddEntity(enemy, true);
    }
    ImGui::SetItemTooltip("Spawns a patrol enemy with two starter waypoints. Add more waypoints in the inspector.");

    if (ImGui::Button("Spawn Terrain"))
    {
        Entity terrain;
        terrain.name = "Terrain";
        terrain.Transform.Position = spawnPosition;
        terrain.Transform.Scale    = Vec3(60.0f, 8.0f, 60.0f);
        terrain.IsTerrain          = true;
        terrain.TerrainGridWidth   = 64;
        terrain.TerrainGridDepth   = 64;
        terrain.CollidesWithPlayer = false;
        // Generate mesh before adding so AddEntity won't assign the shared cube
        TerrainSystem::GenerateTerrainMesh(terrain);
        entityManager.AddEntity(terrain, false);
    }
    ImGui::SetItemTooltip("Spawns a 60x60 heightmap terrain. Configure the heightmap PNG in the inspector, then Regenerate.");

    // Options
    ImGui::Checkbox("Use shared cube mesh", &useSharedCube);
}

void EntityManagerInspector::SpawnNewEntity(EntityManager& entityManager, const Vec3& spawnPosition,
                                           const Vec3& spawnScale, bool useSharedCube)
{
    Entity entity;
    entity.name = "Cube";
    entity.Transform.Position = spawnPosition;
    entity.Transform.Scale = spawnScale;

    if (m_modelPath[0] != '\0')
    {
        std::string pathStr(m_modelPath);
        MeshHandle handle = MeshManager::Instance().LoadMeshSync(pathStr);
        if (handle != 0)
        {
            entity.MeshHandle = handle;
            entity.MeshPath = pathStr;
            entity.name = "Model: " + pathStr;
            entityManager.AddEntity(entity, useSharedCube);
        }
        else
        {
            m_showModelError = true;
            snprintf(m_modelErrorMsg, sizeof(m_modelErrorMsg), "Failed to load model: %s", m_modelPath);
        }
    }
    else
    {
        entityManager.AddEntity(entity, useSharedCube);
    }
}

void EntityManagerInspector::ApplyModelToSelected(EntityManager& entityManager, int selectedIndex)
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(entityManager.Size()))
    {
        return;
    }

    if (m_modelPath[0] == '\0')
    {
        return;
    }

    auto& entity = entityManager.GetAll()[selectedIndex];

    // Release old mesh
    if (entity.MeshHandle != 0)
    {
        MeshManager::Instance().Release(entity.MeshHandle);
    }

    // Load and assign new mesh
    MeshHandle handle = MeshManager::Instance().LoadMeshSync(m_modelPath);
    if (handle != 0)
    {
        entity.MeshHandle = handle;
        entity.MeshPath = std::string(m_modelPath);
        entity.name = "Model: " + std::string(m_modelPath);
    }
    else
    {
        m_showModelError = true;
        snprintf(m_modelErrorMsg, sizeof(m_modelErrorMsg), "Failed to load model: %s", m_modelPath);
    }
}

void EntityManagerInspector::DrawEntityList(EntityManager& entityManager, int& selectedIndex)
{
    auto& entities = entityManager.GetAll();

    ImGui::BeginChild("EntityListScroll", ImVec2(0, ENTITY_LIST_HEIGHT), true);
    ImGui::Columns(2);
    ImGui::SetColumnWidth(1, DELETE_BUTTON_WIDTH);

    for (size_t i = 0; i < entities.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));

        auto& entity = entities[i];
        bool isSelected = (selectedIndex == static_cast<int>(i));

        // Icon based on mesh status
        const char* icon = entity.MeshHandle != 0 ? "(Rendered) " : "(NotRendered) ";
        std::string displayName = icon + entity.name;
        if (entity.IsSpawnPoint)
            displayName = "[SP] " + displayName;
        if (entity.IsTeleporter)
            displayName = "[TP] " + displayName;
        if (entity.IsGoal)
            displayName = "[GOAL] " + displayName;
        if (entity.IsEnemy)
            displayName = "[ENEMY] " + displayName;
        if (entity.IsTerrain)
            displayName = "[TERRAIN] " + displayName;

        if (ImGui::Selectable(displayName.c_str(), isSelected))
        {
            selectedIndex = static_cast<int>(i);
        }

        ImGui::NextColumn();

        ImGui::AlignTextToFramePadding();
        if (ImGui::SmallButton("Delete"))
        {
            entityManager.RemoveAt(i);
            if (selectedIndex == static_cast<int>(i))
                selectedIndex = -1;
            else if (selectedIndex > static_cast<int>(i))
                selectedIndex--;
            
            ImGui::PopID();
            break;
        }

        ImGui::NextColumn();
        ImGui::PopID();
    }

    ImGui::Columns(1);
    ImGui::EndChild();
}

void EntityManagerInspector::DrawFullEntityInspector(EntityManager& entityManager, int selectedIndex)
{
    if (selectedIndex < 0 || selectedIndex >= static_cast<int>(entityManager.Size()))
        return;

    auto& entity = entityManager.GetAll()[selectedIndex];

    DrawEntityInfo(entity);
    DrawEntityTransform(entity);
    DrawEntityMesh(entity);
    DrawEntityMaterial(entity);
    DrawEntityTextures(entity);
}

void EntityManagerInspector::DrawEntityInfo(Entity& entity)
{
    ImGui::Text("Selected: %s", entity.name.c_str());
    ImGui::Separator();

    char nameBuf[128];
    strncpy_s(nameBuf, entity.name.c_str(), sizeof(nameBuf));
    if (ImGui::InputText("Name", nameBuf, sizeof(nameBuf)))
    {
        entity.name = nameBuf;
    }

    // Spawn point tag
    ImGui::Spacing();
    if (entity.IsSpawnPoint)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.2f, 1.0f, 0.3f, 1.0f));
        ImGui::Text("[SP] This entity is a Spawn Point");
        ImGui::PopStyleColor();
    }
    if (ImGui::Checkbox("Spawn Point", &entity.IsSpawnPoint))
    {
        if (entity.IsSpawnPoint)
            entity.CollidesWithPlayer = false;
    }
    ImGui::SetItemTooltip("When Play Mode starts, the player spawns at this entity's position.");

    // Teleporter tag
    ImGui::Spacing();
    if (entity.IsTeleporter)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
        ImGui::Text("[TP] Teleporter  |  Pair ID: %d", entity.TeleporterPairID);
        ImGui::PopStyleColor();
        ImGui::SliderFloat("Teleport Radius", &entity.TeleporterRadius, 0.5f, 10.0f);
        ImGui::SetItemTooltip("Player must be within this distance to trigger the teleport.");
    }

    // Goal tag
    ImGui::Spacing();
    if (entity.IsGoal)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.1f, 1.0f));
        ImGui::Text("[GOAL] This entity is a Goal");
        ImGui::PopStyleColor();
    }
    if (ImGui::Checkbox("Goal", &entity.IsGoal))
    {
    }
    ImGui::SetItemTooltip("Player reaching this entity in Play Mode triggers the Goal Reached screen.");
    if (entity.IsGoal)
    {
        ImGui::SliderFloat("Goal Radius", &entity.GoalRadius, 0.5f, 15.0f);
        ImGui::SetItemTooltip("Player must be within this distance to trigger the goal.");
    }

    // Enemy
    ImGui::Spacing();
    if (entity.IsEnemy)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.3f, 0.3f, 1.0f));
        ImGui::Text("[ENEMY] Patrol Enemy");
        ImGui::PopStyleColor();
    }
    if (ImGui::Checkbox("Enemy", &entity.IsEnemy))
    {
        if (entity.IsEnemy)
            entity.CollidesWithPlayer = false;
    }
    ImGui::SetItemTooltip("This entity patrols between waypoints and sends the player back to spawn on contact.");
    if (entity.IsEnemy)
    {
        ImGui::SliderFloat("Enemy Speed", &entity.EnemySpeed, 0.5f, 20.0f);
        ImGui::SliderFloat("Collision Radius", &entity.EnemyCollisionRadius, 0.1f, 10.0f);
        ImGui::SetItemTooltip("Player is sent to spawn when within this distance of the enemy.");

        const char* patrolModes[] = { "Loop", "Ping-Pong" };
        int modeIdx = static_cast<int>(entity.EnemyPatrolMode);
        if (ImGui::Combo("Patrol Mode", &modeIdx, patrolModes, 2))
            entity.EnemyPatrolMode = static_cast<PatrolMode>(modeIdx);
        ImGui::SetItemTooltip("Loop: jumps back to waypoint 0 after the last. Ping-Pong: reverses direction.");

        ImGui::Spacing();
        ImGui::Text("Waypoints (%zu):", entity.PatrolWaypoints.size());
        for (int w = 0; w < static_cast<int>(entity.PatrolWaypoints.size()); ++w)
        {
            ImGui::PushID(w);
            ImGui::Text("  %d:", w);
            ImGui::SameLine();
            std::string dragLabel = "##WP" + std::to_string(w);
            ImGui::DragFloat3(dragLabel.c_str(), &entity.PatrolWaypoints[w].x, 0.1f);
            ImGui::SameLine();
            if (ImGui::SmallButton("X"))
            {
                entity.PatrolWaypoints.erase(entity.PatrolWaypoints.begin() + w);
                ImGui::PopID();
                break;
            }
            ImGui::PopID();
        }
        if (ImGui::Button("Add Waypoint"))
            entity.PatrolWaypoints.push_back(entity.Transform.Position);
        ImGui::SetItemTooltip("Adds a waypoint at the enemy's current position.");
    }

    // Terrain
    ImGui::Spacing();
    if (entity.IsTerrain)
    {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 0.9f, 0.4f, 1.0f));
        ImGui::Text("[TERRAIN] Heightmap Terrain");
        ImGui::PopStyleColor();
    }
    if (ImGui::Checkbox("Terrain", &entity.IsTerrain))
    {
        if (entity.IsTerrain)
        {
            entity.CollidesWithPlayer = false;
            if (entity.TerrainGridWidth  < 2) entity.TerrainGridWidth  = 64;
            if (entity.TerrainGridDepth  < 2) entity.TerrainGridDepth  = 64;
        }
    }
    ImGui::SetItemTooltip("Turns this entity into a heightmap terrain with height-accurate collisions.");
    if (entity.IsTerrain)
    {
        // Heightmap PNG picker
        static char s_hmPath[260] = "";
        strncpy_s(s_hmPath, entity.TerrainHeightmapPath.c_str(), sizeof(s_hmPath));
        if (ImGui::InputText("Heightmap PNG", s_hmPath, sizeof(s_hmPath)))
            entity.TerrainHeightmapPath = s_hmPath;
        ImGui::SameLine();
        if (ImGui::Button("Browse##HM"))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, sizeof(buf),
                "Images\0*.png;*.jpg;*.bmp\0All\0*.*\0"))
                entity.TerrainHeightmapPath = buf;
        }
        ImGui::SetItemTooltip("Greyscale PNG used as heightmap. Leave empty for procedural rolling hills.");

        ImGui::SliderInt("Grid Width",  &entity.TerrainGridWidth,  4, 256);
        ImGui::SliderInt("Grid Depth",  &entity.TerrainGridDepth,  4, 256);
        ImGui::SetItemTooltip("Mesh subdivision count. Higher = more detail but more vertices.");

        if (ImGui::Button("Regenerate Terrain"))
            TerrainSystem::GenerateTerrainMesh(entity);
        ImGui::SetItemTooltip("Rebuild the terrain mesh from the current heightmap / settings.");
    }

    // Collision
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Checkbox("Collides With Player", &entity.CollidesWithPlayer);
    ImGui::SetItemTooltip("When unchecked, the player passes through this entity. Disable for spawn points, teleporters, and goals.");
}

void EntityManagerInspector::DrawEntityTransform(Entity& entity)
{
    ImGui::Spacing();
    ImGui::Text("Transform");
    ImGui::DragFloat3("Position", &entity.Transform.Position.x, 0.1f);
    ImGui::DragFloat3("Rotation", &entity.Transform.Rotation.x, 1.0f);
    ImGui::DragFloat3("Scale", &entity.Transform.Scale.x, 0.01f);
}

void EntityManagerInspector::DrawEntityMesh(Entity& entity)
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Mesh");

    if (entity.MeshHandle == 0)
    {
        ImGui::Text("No mesh assigned");
        return;
    }

    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
    if (!mesh)
        return;

    std::string meshDisplayName = entity.MeshPath.empty() ? "[Cube]" : entity.MeshPath.substr(entity.MeshPath.find_last_of("/\\") + 1);
    ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Loaded: [%s]", meshDisplayName.c_str());
    ImGui::Text("Vertices: %zu", mesh->Vertices.size());

    if (ImGui::Button("Change Mesh"))
    {
        char buf[1024] = {0};
        if (Platform::OpenFileDialog(buf, sizeof(buf),
            "3D Models\0*.obj;*.gltf;*.glb;*.fbx\0OBJ Files\0*.obj\0GLTF Files\0*.gltf;*.glb\0FBX Files\0*.fbx\0All\0*.*\0"))
        {
            MeshHandle newHandle = MeshManager::Instance().LoadMeshSync(buf);
            if (newHandle != 0)
            {
                MeshManager::Instance().Release(entity.MeshHandle);
                entity.MeshHandle = newHandle;
                entity.MeshPath = buf;
            }
        }
    }

    ImGui::SameLine();
    
    if (ImGui::Button("Remove Mesh"))
    {
        MeshManager::Instance().Release(entity.MeshHandle);
        entity.MeshHandle = 0;
        entity.MeshPath = "";
    }
}

void EntityManagerInspector::DrawEntityMaterial(Entity& entity)
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Material Properties");
    ImGui::SliderFloat("Shininess", &entity.Shininess, 1.0f, 256.0f);
    ImGui::SliderFloat("Alpha", &entity.Alpha, 0.0f, 1.0f);
}

void EntityManagerInspector::DrawEntityTextures(Entity& entity)
{
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Text("Textures");

    DrawTextureOverride(entity, TextureType::Diffuse);
    ImGui::Spacing();
    DrawTextureOverride(entity, TextureType::Specular);
    ImGui::Spacing();
    DrawTextureOverride(entity, TextureType::Normal);
}

void EntityManagerInspector::DrawTextureOverride(Entity& entity, TextureType type)
{
    struct TextureInfo
    {
        const char* label;
        const char* tag;
        bool& hasOverride;
        unsigned int& texture;
        std::string& path;
        int channels;
        GLenum format;
    };

    TextureInfo info = [&]() -> TextureInfo
    {
        switch (type)
        {
            case TextureType::Diffuse:
                return {"Diffuse", "##Diffuse", entity.HasDiffuseTextureOverride, 
                        entity.DiffuseTexture, entity.DiffuseTexturePath, 
                        DIFFUSE_CHANNELS, GL_RGBA};
            case TextureType::Specular:
                return {"Specular", "##Specular", entity.HasSpecularTextureOverride,
                        entity.SpecularTexture, entity.SpecularTexturePath,
                        SPECULAR_CHANNELS, GL_RED};
            case TextureType::Normal:
                return {"Normal", "##Normal", entity.HasNormalTextureOverride,
                        entity.NormalTexture, entity.NormalTexturePath,
                        NORMAL_CHANNELS, GL_RGBA};
            default:
                return {"Unknown", "##Unknown", entity.HasDiffuseTextureOverride,
                        entity.DiffuseTexture, entity.DiffuseTexturePath,
                        DIFFUSE_CHANNELS, GL_RGBA};
        }
    }();

    if (info.hasOverride)
    {
        ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "%s: Override Active", info.label);
        ImGui::Text("  %s", info.path.c_str());
        
        std::string buttonLabel = std::string("Remove Override") + info.tag;
        if (ImGui::Button(buttonLabel.c_str()))
        {
            if (info.texture != 0)
            {
                glDeleteTextures(1, &info.texture);
                info.texture = 0;
            }
            info.hasOverride = false;
            info.path = "";
        }
    }
    else
    {
        ImGui::Text("%s: (using mesh default)", info.label);
        
        std::string buttonLabel = std::string("Set Override") + info.tag;
        if (ImGui::Button(buttonLabel.c_str()))
        {
            char buf[1024] = {0};
            if (Platform::OpenFileDialog(buf, sizeof(buf),
                "Image Files\0*.png;*.jpg;*.jpeg;*.bmp;*.tga\0All\0*.*\0"))
            {
                int width, height;
                unsigned int tex = LoadTextureWithSettings(buf, width, height, info.channels);
                
                if (tex != 0)
                {
                    info.texture = tex;
                    info.path = buf;
                    info.hasOverride = true;
                }
            }
        }
    }
}

unsigned int EntityManagerInspector::LoadTextureWithSettings(const char* path, int& width, int& height, int channels)
{
    // Match OBJ loader behavior - don't flip for manually loaded textures
    // This ensures consistency with how the model's UVs were exported
    stbi_set_flip_vertically_on_load(false);
    
    int actualChannels;
    unsigned char* data = stbi_load(path, &width, &height, &actualChannels, channels);
    
    if (!data)
        return 0;

    unsigned int tex;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    GLenum format = (channels == 1) ? GL_RED : GL_RGBA;
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);

    // Apply graphics settings
    auto& settings = GraphicsSettings::Instance();
    if (settings.EnableMipmaps)
    {
        glGenerateMipmap(GL_TEXTURE_2D);
    }
    settings.ApplyToTexture(tex);

    stbi_image_free(data);
    return tex;
}

void EntityManagerInspector::DrawTextureAssignmentPopup(EntityManager& entityManager, int selectedIndex)
{
    // Check if browsed file is a texture
    if (m_modelPath[0] != '\0')
    {
        std::string path(m_modelPath);
        std::string ext;
        auto pos = path.find_last_of('.');
        if (pos != std::string::npos)
        {
            ext = path.substr(pos + 1);
            std::transform(ext.begin(), ext.end(), ext.begin(),
                         [](unsigned char c) { return std::tolower(c); });
        }

        if ((ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") && selectedIndex >= 0)
        {
            m_pendingTexturePath = path;
            ImGui::OpenPopup("AssignTexturePopup");
            m_modelPath[0] = '\0';
        }
    }

    if (ImGui::BeginPopup("AssignTexturePopup"))
    {
        if (selectedIndex >= 0 && selectedIndex < static_cast<int>(entityManager.Size()))
        {
            auto& entity = entityManager.GetAll()[selectedIndex];

            if (ImGui::MenuItem("Diffuse"))
            {
                if (entity.MeshHandle != 0)
                {
                    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
                    if (mesh)
                    {
                        mesh->LoadTexture(m_pendingTexturePath);
                        mesh->DiffuseTexturePath = m_pendingTexturePath;
                    }
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Specular"))
            {
                if (entity.MeshHandle != 0)
                {
                    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
                    if (mesh)
                    {
                        mesh->LoadSpecularTexture(m_pendingTexturePath);
                        mesh->SpecularTexturePath = m_pendingTexturePath;
                    }
                }
                ImGui::CloseCurrentPopup();
            }

            if (ImGui::MenuItem("Normal"))
            {
                if (entity.MeshHandle != 0)
                {
                    Mesh* mesh = MeshManager::Instance().GetMesh(entity.MeshHandle);
                    if (mesh)
                    {
                        mesh->LoadNormalTexture(m_pendingTexturePath);
                        mesh->NormalTexturePath = m_pendingTexturePath;
                    }
                }
                ImGui::CloseCurrentPopup();
            }
        }

        ImGui::EndPopup();
    }
}

void EntityManagerInspector::DrawModelErrorPopup()
{
    if (m_showModelError)
    {
        ImGui::OpenPopup("Model Load Error");
        if (ImGui::BeginPopupModal("Model Load Error", nullptr, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(m_modelErrorMsg);
            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
                m_showModelError = false;
            }
            ImGui::EndPopup();
        }
    }
}
