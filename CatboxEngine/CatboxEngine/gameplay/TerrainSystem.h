#pragma once
#include <cfloat>

class Entity;

// Terrain utilities: mesh generation and height sampling.
// All methods must be called on the main (OpenGL) thread.
class TerrainSystem
{
public:
    // Generates and uploads a terrain mesh for the given entity.
    // Populates entity.TerrainHeightData and entity.MeshHandle.
    // If TerrainHeightmapPath is empty, procedural rolling hills are used.
    static void GenerateTerrainMesh(Entity& entity);

    // Returns the world-space Y height at (worldX, worldZ) on a terrain entity.
    // Returns -FLT_MAX if the point is outside terrain bounds or the entity is not terrain.
    static float SampleHeight(const Entity& terrain, float worldX, float worldZ);
};
