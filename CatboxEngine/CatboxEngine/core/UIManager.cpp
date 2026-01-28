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
        }
#endif
    }
    if (ImGui::Button("Spawn"))
    {
        Entity e;
        e.name = "Cube";
        e.Transform.Position = spawnPosition;
        e.Transform.Scale = spawnScale;
        if (modelPath[0] != '\0')
        {
            // try load model
            Mesh m;
            if (m.LoadFromOBJ(modelPath))
            {
                m.Upload();
                e.Mesh = m;
                e.name = std::string("Model: ") + modelPath;
            }
        }
        entityManager.Add(e);
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

    ImGui::End();

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

    // Shared mesh option
    ImGui::Begin("Options");
    ImGui::Checkbox("Use shared cube mesh", &useSharedCube);
    ImGui::End();
}

void UIManager::Render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
