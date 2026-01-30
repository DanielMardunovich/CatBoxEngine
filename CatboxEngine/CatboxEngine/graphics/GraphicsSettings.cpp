#include "GraphicsSettings.h"
#include <glad/glad.h>
#include <algorithm>

// Anisotropic filtering extension constants
#ifndef GL_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_TEXTURE_MAX_ANISOTROPY_EXT 0x84FE
#endif
#ifndef GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT
#define GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT 0x84FF
#endif

GraphicsSettings& GraphicsSettings::Instance()
{
    static GraphicsSettings instance;
    return instance;
}

void GraphicsSettings::ApplyToTexture(unsigned int textureID)
{
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Set min filter
    int minFilter = GetGLMinFilter(MinFilter, EnableMipmaps);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter);

    // Set mag filter
    int magFilter = GetGLMagFilter(MagFilter);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, magFilter);

    // Set anisotropic filtering if enabled
    if (AnisotropicFiltering > 1.0f)
    {
        // Check if extension is supported
        float maxAniso = 0.0f;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAniso);
        float aniso = std::min(AnisotropicFiltering, maxAniso);
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, aniso);
    }

    // Set max mipmap level
    if (EnableMipmaps && MaxMipmapLevel >= 0)
    {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, MaxMipmapLevel);
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

int GraphicsSettings::GetGLMinFilter(TextureFilterMode mode, bool useMipmaps)
{
    if (!useMipmaps)
    {
        // No mipmaps - use simple filtering
        if (mode == TextureFilterMode::Nearest)
            return GL_NEAREST;
        else
            return GL_LINEAR;
    }

    // With mipmaps
    switch (mode)
    {
        case TextureFilterMode::Nearest:
            return GL_NEAREST_MIPMAP_NEAREST;
        case TextureFilterMode::Linear:
            return GL_LINEAR;
        case TextureFilterMode::Bilinear:
            return GL_LINEAR_MIPMAP_NEAREST;
        case TextureFilterMode::Trilinear:
        default:
            return GL_LINEAR_MIPMAP_LINEAR;
    }
}

int GraphicsSettings::GetGLMagFilter(TextureFilterMode mode)
{
    // Mag filter doesn't use mipmaps
    if (mode == TextureFilterMode::Nearest)
        return GL_NEAREST;
    else
        return GL_LINEAR;
}
