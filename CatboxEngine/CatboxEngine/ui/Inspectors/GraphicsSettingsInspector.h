#pragma once
#include "../../graphics/GraphicsSettings.h"
#include <array>

class GraphicsSettingsInspector
{
public:
    GraphicsSettingsInspector() = default;
    ~GraphicsSettingsInspector() = default;

    // Disable copy, enable move
    GraphicsSettingsInspector(const GraphicsSettingsInspector&) = delete;
    GraphicsSettingsInspector& operator=(const GraphicsSettingsInspector&) = delete;
    GraphicsSettingsInspector(GraphicsSettingsInspector&&) noexcept = default;
    GraphicsSettingsInspector& operator=(GraphicsSettingsInspector&&) noexcept = default;

    void Draw();

private:
    const char* GetFilterModeName(TextureFilterMode mode);

    // Skybox file path input buffer (ImGui needs mutable char array)
    static constexpr int k_pathBufSize = 256;
    char m_skyboxPath[256] = {};
};
