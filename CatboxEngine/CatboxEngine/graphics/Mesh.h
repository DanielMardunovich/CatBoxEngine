#pragma once
#include <vector>
#include <string>
#include "../resources/Math/Vec3.h"

struct Vertex 
{
	Vec3 Position;
	Vec3 Normal;
	Vec3 UV;
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
};
