#pragma once
#include "Transform.h"
#include "../graphics/MeshManager.h"
#include <string>

// Texture filtering modes
enum class TextureFilter
{
    Nearest,                    // GL_NEAREST
    Linear,                     // GL_LINEAR
    NearestMipmapNearest,      // GL_NEAREST_MIPMAP_NEAREST
    LinearMipmapNearest,       // GL_LINEAR_MIPMAP_NEAREST
    NearestMipmapLinear,       // GL_NEAREST_MIPMAP_LINEAR
    LinearMipmapLinear         // GL_LINEAR_MIPMAP_LINEAR (Trilinear)
};

enum class TextureWrap
{
    Repeat,          // GL_REPEAT
    MirroredRepeat,  // GL_MIRRORED_REPEAT
    ClampToEdge,     // GL_CLAMP_TO_EDGE
    ClampToBorder    // GL_CLAMP_TO_BORDER
};

class Entity
{
public:
    std::string name;
    Transform Transform;
    // store mesh by handle (managed by MeshManager)
    MeshHandle MeshHandle = 0;
    // store mesh path for scene persistence
    std::string MeshPath = "";
    
    // Per-entity texture overrides
    unsigned int DiffuseTexture = 0;
    std::string DiffuseTexturePath = "";
    bool HasDiffuseTextureOverride = false;
    
    unsigned int SpecularTexture = 0;
    std::string SpecularTexturePath = "";
    bool HasSpecularTextureOverride = false;
    
    unsigned int NormalTexture = 0;
    std::string NormalTexturePath = "";
    bool HasNormalTextureOverride = false;
    
    // Material properties
    float Shininess = 32.0f;
    float Alpha = 1.0f;
    
    // MipMap and texture settings
    TextureFilter MinFilter = TextureFilter::LinearMipmapLinear;
    TextureFilter MagFilter = TextureFilter::Linear;
    TextureWrap WrapS = TextureWrap::Repeat;
    TextureWrap WrapT = TextureWrap::Repeat;
    float Anisotropy = 4.0f;  // Anisotropic filtering level (1.0 = off, 16.0 = max)
    bool UseCustomTextureSettings = false;
};
