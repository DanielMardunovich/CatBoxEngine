#pragma once
#include "Transform.h"
#include "../graphics/MeshManager.h"
#include <string>

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
};
