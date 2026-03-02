#pragma once
#include <string>
#include <vector>

class EntityManager;
class RecordTimeSystem;

// ImGui window that lists .scene files from the Scenes/ directory and lets
// the user load one.  Best times from RecordTimeSystem are shown inline.
class LevelSelectMenu
{
public:
    // Draws the level list content inline — call this inside an already-open ImGui window.
    void DrawContents(EntityManager& entityManager, RecordTimeSystem& records);

    // Force a re-scan of the Scenes/ directory on the next Draw call
    void MarkDirty() { m_needsRefresh = true; }

private:
    void RefreshLevelList();

    std::vector<std::string> m_levelFiles;
    int  m_selectedLevel = -1;
    bool m_needsRefresh  = true;
};
