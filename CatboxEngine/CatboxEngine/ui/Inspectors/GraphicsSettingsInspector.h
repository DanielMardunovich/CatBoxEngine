#pragma once
#include "../../graphics/GraphicsSettings.h"

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
};
