#pragma once

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

private:
    GraphicsSettings() = default;
    ~GraphicsSettings() = default;

    // Prevent copying
    GraphicsSettings(const GraphicsSettings&) = delete;
    GraphicsSettings& operator=(const GraphicsSettings&) = delete;
};
