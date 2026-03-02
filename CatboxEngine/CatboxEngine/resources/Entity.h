#pragma once
#include "Transform.h"
#include "../graphics/MeshManager.h"
#include <string>
#include <vector>

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

enum class PatrolMode
{
    Loop,      // After the last waypoint, jump back to the first
    PingPong   // After the last waypoint, reverse direction
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

    // Gameplay tags
    bool IsSpawnPoint = false;
    bool IsPlayer = false;  // Marks the entity used by the player controller (persisted per scene)

    bool IsTeleporter = false;
    int TeleporterPairID = -1;      // Two entities sharing the same ID are linked
    float TeleporterRadius = 2.0f;  // Activation distance

    bool IsGoal = false;
    float GoalRadius = 2.0f;        // Activation distance

    // Collision
    bool CollidesWithPlayer = true;  // When false, the player can walk through this entity

    // Enemy patrol
    bool IsEnemy = false;
    std::vector<Vec3> PatrolWaypoints;
    PatrolMode EnemyPatrolMode = PatrolMode::Loop;
    float EnemySpeed = 3.0f;
    float EnemyCollisionRadius = 1.0f;  // Radius within which the player is sent back to spawn

    // Heightmap terrain (distinct from static meshes)
    bool IsTerrain = false;
    std::string TerrainHeightmapPath = "";  // Optional greyscale PNG; empty = procedural hills
    int TerrainGridWidth = 64;              // Mesh subdivisions along X
    int TerrainGridDepth = 64;              // Mesh subdivisions along Z
    // Runtime: normalised [0,1] height samples rebuilt from heightmap on load — NOT serialised
    std::vector<float> TerrainHeightData;
    int TerrainHeightDataWidth  = 0;
    int TerrainHeightDataDepth  = 0;
};
