#pragma once
#include <string>

enum class TextureFilterMode
{
    Nearest = 0,        // GL_NEAREST (pixelated)
    Linear = 1,         // GL_LINEAR (smooth)
    Bilinear = 2,       // GL_LINEAR_MIPMAP_NEAREST
    Trilinear = 3       // GL_LINEAR_MIPMAP_LINEAR (best quality)
};

class GraphicsSettings
{
public:
    static GraphicsSettings& Instance();

    // Texture settings
    bool EnableMipmaps = true;
    TextureFilterMode MinFilter = TextureFilterMode::Trilinear;
    TextureFilterMode MagFilter = TextureFilterMode::Linear;
    float AnisotropicFiltering = 16.0f;  // 0 = disabled, 1-16 = level
    int MaxMipmapLevel = 1000;  // -1 = all levels

    // Apply settings to a texture
    void ApplyToTexture(unsigned int textureID);

    // Get OpenGL enum for filter mode
    int GetGLMinFilter(TextureFilterMode mode, bool useMipmaps);
    int GetGLMagFilter(TextureFilterMode mode);

    // Skybox settings
    bool  SkyboxEnabled      = true;
    bool  SkyboxProcedural   = true;   // false = use mesh file below
    float SkyColorTop[3]     = { 0.10f, 0.40f, 0.80f };
    float SkyColorHorizon[3] = { 0.50f, 0.70f, 0.90f };
    float SkyColorBottom[3]  = { 0.30f, 0.25f, 0.15f };
    // Path to a skybox mesh file (GLTF, GLB, OBJ)
    std::string SkyboxFilePath;
    bool  SkyboxFileDirty = false;  // set to true to trigger a reload in RenderPipeline

private:
    GraphicsSettings() = default;
    ~GraphicsSettings() = default;

    // Prevent copying
    GraphicsSettings(const GraphicsSettings&) = delete;
    GraphicsSettings& operator=(const GraphicsSettings&) = delete;
};
