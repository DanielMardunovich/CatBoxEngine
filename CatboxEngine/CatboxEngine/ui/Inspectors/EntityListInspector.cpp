#include "EntityListInspector.h"
#include "../../resources/EntityManager.h"
#include "../../resources/Entity.h"
#include "imgui.h"
#include <string>

void EntityListInspector::Draw(EntityManager& entityManager, int& selectedIndex)
{
    ImGui::Begin("Entity List");

    ImGui::Text("Entities: %zu", entityManager.Size());
    ImGui::Separator();

    // Entity list with selection and deletion
    auto& entities = entityManager.GetAll();
    
    for (size_t i = 0; i < entities.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        
        bool isSelected = (selectedIndex == static_cast<int>(i));
        
        // Entity name as selectable
        if (ImGui::Selectable(entities[i].name.c_str(), isSelected, 0, ImVec2(0, 0)))
        {
            selectedIndex = static_cast<int>(i);
        }
        
        // Delete button on same line
        ImGui::SameLine();
        if (ImGui::SmallButton("Delete"))
        {
            entityManager.RemoveAt(i);
            
            // Update selection index
            if (selectedIndex == static_cast<int>(i))
            {
                selectedIndex = -1;
            }
            else if (selectedIndex > static_cast<int>(i))
            {
                selectedIndex -= 1;
            }
            
            ImGui::PopID();
            break; // List changed, break to avoid iterator invalidation
        }
        
        ImGui::PopID();
    }

    ImGui::End();
}
