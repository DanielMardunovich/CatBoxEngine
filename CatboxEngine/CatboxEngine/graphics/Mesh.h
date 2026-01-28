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

struct Mesh
{
public:
	std::vector<Vertex> Vertices;
	std::vector<uint32_t> Indices;

	uint32_t VAO = 0;
	uint32_t VBO = 0;
	uint32_t EBO = 0;

	void Upload();
	void Draw() const;
    bool LoadFromOBJ(const std::string& path);
    bool LoadFromGLTF(const std::string& path);
    // simple material
    Vec3 DiffuseColor{0.8f, 0.8f, 0.9f};
    // texture handle
    unsigned int DiffuseTexture = 0;
    bool HasDiffuseTexture = false;
    // path for texture (for UI/info)
    std::string DiffuseTexturePath;
    // specular map
    unsigned int SpecularTexture = 0;
    bool HasSpecularTexture = false;
    std::string SpecularTexturePath;
    // normal map
    unsigned int NormalTexture = 0;
    bool HasNormalTexture = false;
    std::string NormalTexturePath;
    // material parameters
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
