#include "GraphicsSettingsInspector.h"
#include "imgui.h"

void GraphicsSettingsInspector::Draw()
{
    auto& settings = GraphicsSettings::Instance();
    
    ImGui::Begin("Graphics Settings");
    
    ImGui::Text("Texture Quality Settings");
    ImGui::Separator();
    
    // Mipmap enable/disable
    bool mipmapsChanged = ImGui::Checkbox("Enable Mipmaps", &settings.EnableMipmaps);
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Mipmaps improve texture quality at distance and reduce aliasing.\nDisabling may improve performance but reduce quality.");
    }
    
    ImGui::Spacing();
    
    // Min filter mode
    const char* minFilterModes[] = { "Nearest (Pixelated)", "Linear (Smooth)", "Bilinear", "Trilinear (Best)" };
    int currentMinFilter = static_cast<int>(settings.MinFilter);
    if (ImGui::Combo("Min Filter", &currentMinFilter, minFilterModes, 4))
    {
        settings.MinFilter = static_cast<TextureFilterMode>(currentMinFilter);
        mipmapsChanged = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Filtering when texture is smaller than screen space.\nTrilinear provides best quality with mipmaps.");
    }
    
    // Mag filter mode
    const char* magFilterModes[] = { "Nearest (Pixelated)", "Linear (Smooth)" };
    int currentMagFilter = (settings.MagFilter == TextureFilterMode::Nearest) ? 0 : 1;
    if (ImGui::Combo("Mag Filter", &currentMagFilter, magFilterModes, 2))
    {
        settings.MagFilter = (currentMagFilter == 0) ? TextureFilterMode::Nearest : TextureFilterMode::Linear;
        mipmapsChanged = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Filtering when texture is larger than screen space.");
    }
    
    ImGui::Spacing();
    
    // Anisotropic filtering
    float aniso = settings.AnisotropicFiltering;
    if (ImGui::SliderFloat("Anisotropic Filtering", &aniso, 1.0f, 16.0f, "%.0fx"))
    {
        settings.AnisotropicFiltering = aniso;
        mipmapsChanged = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Improves texture quality at steep angles.\nHigher values = better quality but slower.\n1x = disabled");
    }
    
    ImGui::Spacing();
    
    // Max mipmap level
    int maxLevel = settings.MaxMipmapLevel;
    if (ImGui::SliderInt("Max Mipmap Level", &maxLevel, 0, 10))
    {
        settings.MaxMipmapLevel = maxLevel;
        mipmapsChanged = true;
    }
    if (ImGui::IsItemHovered())
    {
        ImGui::SetTooltip("Limits how many mipmap levels to use.\nLower = better performance, higher = better quality at distance.");
    }
    
    ImGui::Spacing();
    ImGui::Separator();
    
    // Info about changes
    if (mipmapsChanged)
    {
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Note: Changes apply to newly loaded textures.");
        ImGui::Text("Reload textures or scene to see full effect.");
    }
    
    ImGui::Spacing();
    
    // Current settings summary
    if (ImGui::CollapsingHeader("Current Settings Summary"))
    {
        ImGui::Text("Mipmaps: %s", settings.EnableMipmaps ? "Enabled" : "Disabled");
        ImGui::Text("Min Filter: %s", GetFilterModeName(settings.MinFilter));
        ImGui::Text("Mag Filter: %s", GetFilterModeName(settings.MagFilter));
        ImGui::Text("Anisotropic: %.0fx", settings.AnisotropicFiltering);
        ImGui::Text("Max Mip Level: %d", settings.MaxMipmapLevel);
    }
    
    ImGui::End();
}

const char* GraphicsSettingsInspector::GetFilterModeName(TextureFilterMode mode)
{
    switch (mode)
    {
        case TextureFilterMode::Nearest: return "Nearest";
        case TextureFilterMode::Linear: return "Linear";
        case TextureFilterMode::Bilinear: return "Bilinear";
        case TextureFilterMode::Trilinear: return "Trilinear";
        default: return "Unknown";
    }
}
