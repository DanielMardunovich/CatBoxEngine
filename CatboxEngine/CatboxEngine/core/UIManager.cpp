#include "UIManager.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "../resources/EntityManager.h"
#include "../resources/Entity.h"

void UIManager::NewFrame()
{
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void UIManager::Draw(EntityManager& entityManager, Vec3& spawnPosition, Vec3& spawnScale, float deltaTime)
{
    ImGui::Begin("Hello, Catbox!");
    ImGui::Text("This is a simple window.");

    ImGui::Separator();
    ImGui::Text("Spawn Cube");
    ImGui::InputFloat3("Position", &spawnPosition.x);
    ImGui::InputFloat3("Scale", &spawnScale.x);
    if (ImGui::Button("Spawn"))
    {
        Entity e;
        e.name = "Cube";
        e.Transform.Position = spawnPosition;
        e.Transform.Scale = spawnScale;
        entityManager.Add(e);
    }

    ImGui::Separator();
    ImGui::Text("Entities (%d)", (int)entityManager.Size());
    ImGui::BeginChild("EntityList", ImVec2(0, 200), true);
    auto& list = entityManager.GetAll();
    for (size_t i = 0; i < list.size(); ++i)
    {
        ImGui::PushID((int)i);
        ImGui::Text("%s", list[i].name.c_str());
        ImGui::SameLine();
        if (ImGui::SmallButton("Delete"))
        {
            entityManager.RemoveAt(i);
            ImGui::PopID();
            break; // changed list, break out to avoid iterator invalidation
        }
        ImGui::PopID();
    }
    ImGui::EndChild();

    // Display timing
    ImGui::Separator();
    ImGui::Text("Delta: %.4f", deltaTime);
    ImGui::Text("FPS: %.1f", 1.0f / deltaTime);

    ImGui::End();
}

void UIManager::Render()
{
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}
