#include "LevelSelectMenu.h"
#include "../../gameplay/RecordTimeSystem.h"
#include "../../resources/SceneManager.h"
#include "../../resources/EntityManager.h"
#include "imgui.h"
#include <filesystem>
#include <algorithm>

namespace fs = std::filesystem;

void LevelSelectMenu::RefreshLevelList()
{
    m_levelFiles.clear();
    m_needsRefresh = false;

    std::error_code ec;
    if (!fs::is_directory("Scenes", ec))
        return;

    for (const auto& entry : fs::directory_iterator("Scenes", ec))
    {
        if (entry.is_regular_file() && entry.path().extension() == ".scene")
        {
            // Normalise to forward-slash for consistent record key matching
            std::string p = entry.path().string();
            std::replace(p.begin(), p.end(), '\\', '/');
            m_levelFiles.push_back(std::move(p));
        }
    }

    std::sort(m_levelFiles.begin(), m_levelFiles.end());
}

void LevelSelectMenu::DrawContents(EntityManager& entityManager, RecordTimeSystem& records)
{
    if (m_needsRefresh)
        RefreshLevelList();

    if (ImGui::Button("Refresh"))
        MarkDirty();

    ImGui::SameLine();
    ImGui::TextDisabled("%zu level(s) found", m_levelFiles.size());

    if (m_levelFiles.empty())
    {
        ImGui::Spacing();
        ImGui::TextDisabled("No .scene files found in Scenes/");
        ImGui::TextDisabled("Save a scene to the Scenes/ folder first.");
        return;
    }

    ImGui::BeginChild("LevelList", ImVec2(0.0f, 150.0f), true);

    for (int i = 0; i < static_cast<int>(m_levelFiles.size()); ++i)
    {
        ImGui::PushID(i);
        const std::string& path = m_levelFiles[i];

        std::string displayName = fs::path(path).stem().string();
        float       best        = records.GetBestTime(path);
        std::string timeStr     = RecordTimeSystem::FormatTime(best);

        bool isSelected = (m_selectedLevel == i);
        if (isSelected)
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.3f, 1.0f, 0.3f, 1.0f));

        std::string label = displayName + "  [" + timeStr + "]";
        if (ImGui::Selectable(label.c_str(), isSelected))
            m_selectedLevel = i;

        if (isSelected)
            ImGui::PopStyleColor();

        ImGui::PopID();
    }

    ImGui::EndChild();

    bool hasSelection = (m_selectedLevel >= 0 &&
                         m_selectedLevel < static_cast<int>(m_levelFiles.size()));

    if (!hasSelection)
        ImGui::BeginDisabled();

    if (ImGui::Button("Load Level", ImVec2(-1.0f, 0.0f)) && hasSelection)
    {
        const std::string& path = m_levelFiles[m_selectedLevel];
        auto& sceneMgr = SceneManager::Instance();
        SceneID id = sceneMgr.LoadScene(path);
        if (id != 0)
            sceneMgr.SetActiveScene(id, entityManager);
    }

    if (!hasSelection)
        ImGui::EndDisabled();
}
