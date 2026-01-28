#pragma once
#include <vector>
#include <string>
#include "../resources/Math/Vec3.h"

struct Vertex 
{
	Vec3 Position;
	Vec3 Normal;
	Vec3 UV;
	Vec3 Tangent;
};

// Morph target (blend shape) data
struct MorphTarget
{
	std::string Name;
	std::vector<Vec3> PositionDeltas;  // Position offsets
	std::vector<Vec3> NormalDeltas;    // Normal offsets
	std::vector<Vec3> TangentDeltas;   // Tangent offsets (optional)
	float Weight = 0.0f;               // Current weight (0-1)
};

// SubMesh represents geometry with a single material
struct SubMesh
{
	std::vector<uint32_t> Indices;
	uint32_t BaseVertex = 0;  // offset into parent mesh's vertex buffer
	
	// Material properties
	std::string MaterialName;
	Vec3 DiffuseColor{0.8f, 0.8f, 0.9f};
	Vec3 SpecularColor{0.2f, 0.2f, 0.2f};
	float Shininess = 16.0f;
	float Alpha = 1.0f;
	
	// Textures
	unsigned int DiffuseTexture = 0;
	bool HasDiffuseTexture = false;
	std::string DiffuseTexturePath;
	
	unsigned int SpecularTexture = 0;
	bool HasSpecularTexture = false;
	std::string SpecularTexturePath;
	
	unsigned int NormalTexture = 0;
	bool HasNormalTexture = false;
	std::string NormalTexturePath;
	
	// OpenGL buffer for this submesh's indices
	uint32_t EBO = 0;
};

struct Mesh
{
public:
	std::vector<Vertex> Vertices;
	std::vector<uint32_t> Indices;  // Legacy: used when there's only 1 material
	std::vector<SubMesh> SubMeshes;  // Multiple materials
	
	// Morph targets / Blend shapes
	std::vector<MorphTarget> MorphTargets;
	std::vector<Vertex> BaseVertices;  // Original vertices for morph target blending

	uint32_t VAO = 0;
	uint32_t VBO = 0;
	uint32_t EBO = 0;  // Legacy: used when there's only 1 material
	
	// Bounding box for frustum culling
	Vec3 BoundsMin{FLT_MAX, FLT_MAX, FLT_MAX};
	Vec3 BoundsMax{-FLT_MAX, -FLT_MAX, -FLT_MAX};

	void Upload();
	void Draw() const;
    bool LoadFromOBJ(const std::string& path);
    bool LoadFromGLTF(const std::string& path);
    
    // Morph target control
    void SetMorphTargetWeight(size_t index, float weight);
    void SetMorphTargetWeight(const std::string& name, float weight);
    void UpdateMorphTargets();  // Apply current weights
    void CalculateBounds();     // Calculate bounding box
    
    // Validation and debugging
    bool ValidateVertexData() const;
    void PrintDebugInfo() const;
    
    // Memory tracking
    size_t GetCPUMemoryUsage() const;
    size_t GetGPUMemoryUsage() const;
    size_t GetTotalMemoryUsage() const { return GetCPUMemoryUsage() + GetGPUMemoryUsage(); }
    
    // Legacy single-material properties (used when SubMeshes.empty())
    Vec3 DiffuseColor{0.8f, 0.8f, 0.9f};
    unsigned int DiffuseTexture = 0;
    bool HasDiffuseTexture = false;
    std::string DiffuseTexturePath;
    unsigned int SpecularTexture = 0;
    bool HasSpecularTexture = false;
    std::string SpecularTexturePath;
    unsigned int NormalTexture = 0;
    bool HasNormalTexture = false;
    std::string NormalTexturePath;
    Vec3 SpecularColor{0.2f,0.2f,0.2f};
    float Shininess = 16.0f;
    float Alpha = 1.0f;
    
    // Load a texture from file and assign as diffuse
    bool LoadTexture(const std::string& path);
    void UnloadTexture();
    bool LoadSpecularTexture(const std::string& path);
    void UnloadSpecularTexture();
    bool LoadNormalTexture(const std::string& path);
    void UnloadNormalTexture();
};
