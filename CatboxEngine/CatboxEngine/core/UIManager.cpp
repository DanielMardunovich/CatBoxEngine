#include "UIManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"
#include "../ui/Inspectors/EntityInspector.h"
#include <string>
#include "../resources/Camera.h"
#include "../ui/Inspectors/CameraInspector.h"
#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#endif
#include <filesystem>

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
#if defined(_WIN32)
        OPENFILENAMEA ofn;
        CHAR szFile[260] = {0};
        ZeroMemory(&ofn, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = NULL;
        ofn.lpstrFile = szFile;
        ofn.nMaxFile = sizeof(szFile);
        ofn.lpstrFilter = "OBJ Files\0*.obj\0All\0*.*\0";
        ofn.nFilterIndex = 1;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;
        if (GetOpenFileNameA(&ofn))
        {
            strncpy_s(modelPath, ofn.lpstrFile, sizeof(modelPath));
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
                        entityManager.GetAll()[selectedIndex].Mesh.LoadTexture(sel);
                    }
                }
        }
#endif
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
            // try load model (try glTF first, then OBJ)
            Mesh m;
            bool loaded = false;
            std::string pathStr(modelPath);
            auto extPos = pathStr.find_last_of('.');
            std::string ext = extPos != std::string::npos ? pathStr.substr(extPos+1) : std::string();
            if (!ext.empty())
            {
                for (auto &c : ext) c = (char)tolower(c);
            }

            if (ext == "gltf" || ext == "glb")
            {
                if (m.LoadFromGLTF(pathStr)) loaded = true;
            }
            else
            {
                if (m.LoadFromOBJ(pathStr)) loaded = true;
            }

            if (loaded)
            {
                m.Upload();
                e.Mesh = m;
                e.name = std::string("Model: ") + modelPath;
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
            Mesh m;
            if (m.LoadFromOBJ(modelPath))
            {
                m.Upload();
                entityManager.GetAll()[selectedIndex].Mesh = m;
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
