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
    ImGui::BeginChild("EntityListScroll", ImVec2(0, 0), true);
    
    auto& entities = entityManager.GetAll();
    ImGui::Columns(2);
    ImGui::SetColumnWidth(1, 90.0f);
    
    for (size_t i = 0; i < entities.size(); ++i)
    {
        ImGui::PushID(static_cast<int>(i));
        
        bool isSelected = (selectedIndex == static_cast<int>(i));
        
        // Left column: selectable entity name
        if (ImGui::Selectable(entities[i].name.c_str(), isSelected))
        {
            selectedIndex = static_cast<int>(i);
        }
        
        ImGui::NextColumn();
        
        // Right column: delete button
        ImGui::AlignTextToFramePadding();
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
        
        ImGui::NextColumn();
        ImGui::PopID();
    }
    
    ImGui::Columns(1);
    ImGui::EndChild();

    ImGui::End();
}
