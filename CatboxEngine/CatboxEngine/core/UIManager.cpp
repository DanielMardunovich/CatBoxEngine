#include "UIManager.h"
#include "Platform.h"
#include "MemoryTracker.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include "../ui/Inspectors/EntityInspector.h"
#include "../resources/Camera.h"
#include "../ui/Inspectors/CameraInspector.h"
#include "../graphics/MeshManager.h"
#include <string>

void UIManager::NewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, float deltaTime, int& selectedIndex, Camera& camera, bool& useSharedCube)
{
    ImGui::Begin("Hello, Catbox!");
    ImGui::Text("This is a simple window.");

    ImGui::Separator();
    ImGui::Text("Spawn Cube");
    ImGui::InputFloat3("Position", &spawnPosition.x);
    ImGui::InputFloat3("Scale", &spawnScale.x);
    static char modelPath[260] = "";
    ImGui::InputText("Model Path", modelPath, IM_ARRAYSIZE(modelPath));
    ImGui::SameLine();
    if (ImGui::Button("Browse..."))
    {
        char szFile[260] = {0};
        if (Platform::OpenFileDialog(szFile, sizeof(szFile), "3D Models\0*.obj;*.gltf;*.glb\0OBJ Files\0*.obj\0GLTF Files\0*.gltf;*.glb\0All\0*.*\0"))
        {
            strncpy_s(modelPath, szFile, sizeof(modelPath));
            // if selected file is an image, preview and assign to selected entity texture
            std::string sel(modelPath);
            std::string ext;
            auto p = sel.find_last_of('.');
            if (p != std::string::npos) ext = sel.substr(p+1);
            for (auto &c : ext) c = (char)tolower(c);
            if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp")
            {
                if (selectedIndex >= 0)
                {
                    // Show a small popup to let the user choose which map to assign
                    ImGui::OpenPopup("AssignTexturePopup");
                    // store selected path into a temp char buffer in UIManager scope
                    static std::string pendingTexPath;
                    pendingTexPath = sel;
                    if (ImGui::BeginPopup("AssignTexturePopup"))
                    {
                        if (ImGui::MenuItem("Diffuse"))
                        {
                            if (entityManager.GetAll()[selectedIndex].MeshHandle != 0)
                            {
                                Mesh* mm = MeshManager::Instance().GetMesh(entityManager.GetAll()[selectedIndex].MeshHandle);
                                if (mm) { mm->LoadTexture(pendingTexPath); mm->DiffuseTexturePath = pendingTexPath; }
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::MenuItem("Specular"))
                        {
                            if (entityManager.GetAll()[selectedIndex].MeshHandle != 0)
                            {
                                Mesh* mm = MeshManager::Instance().GetMesh(entityManager.GetAll()[selectedIndex].MeshHandle);
                                if (mm) { mm->LoadSpecularTexture(pendingTexPath); mm->SpecularTexturePath = pendingTexPath; }
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        if (ImGui::MenuItem("Normal"))
                        {
                            if (entityManager.GetAll()[selectedIndex].MeshHandle != 0)
                            {
                                Mesh* mm = MeshManager::Instance().GetMesh(entityManager.GetAll()[selectedIndex].MeshHandle);
                                if (mm) { mm->LoadNormalTexture(pendingTexPath); mm->NormalTexturePath = pendingTexPath; }
                            }
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }
                }
            }
        }
    }
    static bool showModelError = false;
    static char modelErrorMsg[512] = "";

    if (ImGui::Button("Spawn"))
    {
        Entity e;
        e.name = "Cube";
        e.Transform.Position = spawnPosition;
        e.Transform.Scale = spawnScale;
        if (modelPath[0] != '\0')
        {
            // load model synchronously via MeshManager
            std::string pathStr(modelPath);
            MeshHandle h = MeshManager::Instance().LoadMeshSync(pathStr);
            if (h != 0)
            {
                e.MeshHandle = h;
                e.name = std::string("Model: ") + pathStr;
                entityManager.AddEntity(e, useSharedCube);
            }
            else
            {
                // show error popup
                showModelError = true;
                snprintf(modelErrorMsg, sizeof(modelErrorMsg), "Failed to load model: %s", modelPath);
            }
        }
        else
        {
            entityManager.AddEntity(e, useSharedCube);
        }
    }
    ImGui::SameLine();
    if (ImGui::Button("Apply Model to Selected") && selectedIndex >= 0)
    {
        if (modelPath[0] != '\0')
        {
            // Release old handle first
            if (entityManager.GetAll()[selectedIndex].MeshHandle != 0)
            {
                MeshManager::Instance().Release(entityManager.GetAll()[selectedIndex].MeshHandle);
            }
            
            // Load and assign new mesh
            MeshHandle h = MeshManager::Instance().LoadMeshSync(modelPath);
            if (h != 0)
            {
                entityManager.GetAll()[selectedIndex].MeshHandle = h;
                entityManager.GetAll()[selectedIndex].name = std::string("Model: ") + modelPath;
            }
            else
            {
                showModelError = true;
                snprintf(modelErrorMsg, sizeof(modelErrorMsg), "Failed to load model: %s", modelPath);
            }
        }
    }

    ImGui::Separator();
    ImGui::Text("Entities (%d)", (int)entityManager.Size());
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);
    auto& list = entityManager.GetAll();
    ImGui::Columns(2);
    // reserve a small fixed width for the action column (delete button)
    ImGui::SetColumnWidth(1, 90.0f);
    for (size_t i = 0; i < list.size(); ++i)
    {
        ImGui::PushID((int)i);
        bool isSelected = (selectedIndex == (int)i);
        // Left column: selectable name (do NOT span into the action column)
        if (ImGui::Selectable(list[i].name.c_str(), isSelected))
        {
            selectedIndex = (int)i;
        }
        ImGui::NextColumn();

        // Right column: delete button
        ImGui::AlignTextToFramePadding();
        std::string btnId = std::string("Delete##") + std::to_string(i);
        if (ImGui::SmallButton(btnId.c_str()))
        {
            entityManager.RemoveAt(i);
            if (selectedIndex == (int)i) selectedIndex = -1;
            else if (selectedIndex > (int)i) selectedIndex -= 1;
            ImGui::PopID();
            break; // changed list, break out to avoid iterator invalidation
        }

        ImGui::NextColumn();
        ImGui::PopID();
    }
    ImGui::Columns(1);
    ImGui::EndChild();

    // Display timing
    ImGui::Separator();
    ImGui::Text("Delta: %.4f", deltaTime);
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);

    // Memory statistics
    ImGui::Separator();
    ImGui::Text("Memory Stats");
    #if TRACK_MEMORY
    auto& memTracker = MemoryTracker::Instance();
    ImGui::Text("Tracked: %.2f MB", memTracker.GetCurrentUsage() / (1024.0f * 1024.0f));
    ImGui::Text("Allocations: %zu", memTracker.GetActiveAllocations());
    #endif
    
    // Mesh memory (always available)
    auto& meshMgr = MeshManager::Instance();
    float meshCPU = meshMgr.GetTotalCPUMemory() / (1024.0f * 1024.0f);
    float meshGPU = meshMgr.GetTotalGPUMemory() / (1024.0f * 1024.0f);
    ImGui::Text("Meshes: %zu (%.2f MB CPU, %.2f MB GPU)", 
        meshMgr.GetMeshCount(), meshCPU, meshGPU);
    
    #if TRACK_MEMORY
    if (ImGui::Button("Print Report"))
    {
        memTracker.PrintMemoryReport();
    }
    ImGui::SameLine();
    if (ImGui::Button("Check Leaks"))
    {
        memTracker.CheckForLeaks();
    }
    #endif

    // Options moved into main window
    ImGui::Checkbox("Use shared cube mesh", &useSharedCube);

    ImGui::End();

    // error popup
    if (showModelError)
    {
        ImGui::OpenPopup("Model Load Error");
        if (ImGui::BeginPopupModal("Model Load Error", NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::TextUnformatted(modelErrorMsg);
            if (ImGui::Button("OK"))
            {
                ImGui::CloseCurrentPopup();
                showModelError = false;
            }
            ImGui::EndPopup();
        }
    }

    // Inspector window
    if (selectedIndex >= 0 && selectedIndex < (int)entityManager.Size())
    {
        EntityInspector inspector;
        auto& ent = entityManager.GetAll()[selectedIndex];
        inspector.Draw(ent);
    }

    // Camera inspector window
    CameraInspector camInspector;
    camInspector.Draw(camera);

    // Options window removed; options are now in the main window
}

void UIManager::Render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
