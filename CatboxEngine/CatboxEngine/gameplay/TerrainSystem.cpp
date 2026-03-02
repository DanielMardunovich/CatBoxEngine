#include "TerrainSystem.h"
#include "../resources/Entity.h"
#include "../graphics/MeshManager.h"
#include "../graphics/Mesh.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cmath>
#include <cfloat>
#include <algorithm>
#include <iostream>

// stb_image declarations only — STB_IMAGE_IMPLEMENTATION is in Mesh.cpp via tiny_gltf.h
#include "../Dependencies/stb_image.h"

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

// Bilinear-sample a flat height array at normalised UV [0,1].
static float SampleHeightBilinear(const std::vector<float>& data, int w, int d,
                                   float u, float v)
{
    u = std::max(0.0f, std::min(1.0f, u));
    v = std::max(0.0f, std::min(1.0f, v));

    float fx = u * (w - 1);
    float fz = v * (d - 1);
    int   ix = std::min(static_cast<int>(fx), w - 2);
    int   iz = std::min(static_cast<int>(fz), d - 2);
    float tx = fx - ix;
    float tz = fz - iz;

    float h00 = data[ iz      * w + ix    ];
    float h10 = data[ iz      * w + ix + 1];
    float h01 = data[(iz + 1) * w + ix    ];
    float h11 = data[(iz + 1) * w + ix + 1];

    return (h00 + (h10 - h00) * tx) * (1.0f - tz)
         + (h01 + (h11 - h01) * tx) * tz;
}

// ---------------------------------------------------------------------------
// TerrainSystem::GenerateTerrainMesh
// ---------------------------------------------------------------------------

void TerrainSystem::GenerateTerrainMesh(Entity& entity)
{
    if (!entity.IsTerrain) return;

    const int gw = std::max(entity.TerrainGridWidth,  2);
    const int gd = std::max(entity.TerrainGridDepth,  2);
    const int vw = gw + 1;
    const int vd = gd + 1;

    // --- 1. Build height data ---
    bool loadedFromFile = false;

    if (!entity.TerrainHeightmapPath.empty())
    {
        int imgW, imgH, ch;
        unsigned char* imgData = stbi_load(entity.TerrainHeightmapPath.c_str(),
                                           &imgW, &imgH, &ch, 1);
        if (imgData)
        {
            entity.TerrainHeightDataWidth = imgW;
            entity.TerrainHeightDataDepth = imgH;
            entity.TerrainHeightData.resize(static_cast<size_t>(imgW * imgH));
            for (int i = 0; i < imgW * imgH; ++i)
                entity.TerrainHeightData[i] = imgData[i] / 255.0f;
            stbi_image_free(imgData);
            loadedFromFile = true;
        }
        else
        {
            std::cerr << "TerrainSystem: failed to load heightmap \""
                      << entity.TerrainHeightmapPath << "\", using procedural fallback.\n";
        }
    }

    if (!loadedFromFile)
    {
        // Procedural rolling hills: two overlapping sine waves
        entity.TerrainHeightDataWidth = vw;
        entity.TerrainHeightDataDepth = vd;
        entity.TerrainHeightData.resize(static_cast<size_t>(vw * vd));

        for (int iz = 0; iz < vd; ++iz)
        {
            for (int ix = 0; ix < vw; ++ix)
            {
                float u = static_cast<float>(ix) / gw;
                float v = static_cast<float>(iz) / gd;
                // Two frequency sine waves blended together
                float h = (std::sin(u * 3.14159f * 3.0f) * std::cos(v * 3.14159f * 2.0f)
                          + std::sin(u * 3.14159f * 1.5f + 0.8f) * std::cos(v * 3.14159f * 3.5f))
                          * 0.15f + 0.2f;
                entity.TerrainHeightData[iz * vw + ix] = std::max(0.0f, std::min(1.0f, h));
            }
        }
    }

    // --- 2. Build vertex and index buffers ---
    Mesh mesh;
    mesh.Vertices.reserve(static_cast<size_t>(vw * vd));
    mesh.Indices.reserve(static_cast<size_t>(gw * gd * 6));

    for (int iz = 0; iz < vd; ++iz)
    {
        for (int ix = 0; ix < vw; ++ix)
        {
            float u = static_cast<float>(ix) / gw;
            float v = static_cast<float>(iz) / gd;
            float h = SampleHeightBilinear(entity.TerrainHeightData,
                                           entity.TerrainHeightDataWidth,
                                           entity.TerrainHeightDataDepth, u, v);
            Vertex vert;
            vert.Position = { u - 0.5f, h, v - 0.5f };
            vert.UV       = { u * 8.0f, v * 8.0f, 0.0f };  // tiled UVs
            vert.Normal   = { 0.0f, 1.0f, 0.0f };           // recalculated below
            vert.Tangent  = { 1.0f, 0.0f, 0.0f };
            mesh.Vertices.push_back(vert);
        }
    }

    for (int iz = 0; iz < gd; ++iz)
    {
        for (int ix = 0; ix < gw; ++ix)
        {
            uint32_t a = static_cast<uint32_t>( iz      * vw + ix    );
            uint32_t b = static_cast<uint32_t>( iz      * vw + ix + 1);
            uint32_t c = static_cast<uint32_t>((iz + 1) * vw + ix    );
            uint32_t d = static_cast<uint32_t>((iz + 1) * vw + ix + 1);
            mesh.Indices.push_back(a);
            mesh.Indices.push_back(c);
            mesh.Indices.push_back(b);
            mesh.Indices.push_back(b);
            mesh.Indices.push_back(c);
            mesh.Indices.push_back(d);
        }
    }

    // --- 3. Smooth normals (average of adjacent triangle face normals) ---
    std::vector<glm::vec3> normals(mesh.Vertices.size(), glm::vec3(0.0f));
    for (size_t i = 0; i + 2 < mesh.Indices.size(); i += 3)
    {
        uint32_t i0 = mesh.Indices[i], i1 = mesh.Indices[i + 1], i2 = mesh.Indices[i + 2];
        auto toGlm = [&](const Vec3& p) { return glm::vec3(p.x, p.y, p.z); };
        glm::vec3 n = glm::cross(toGlm(mesh.Vertices[i1].Position) - toGlm(mesh.Vertices[i0].Position),
                                 toGlm(mesh.Vertices[i2].Position) - toGlm(mesh.Vertices[i0].Position));
        normals[i0] += n;
        normals[i1] += n;
        normals[i2] += n;
    }
    for (size_t i = 0; i < mesh.Vertices.size(); ++i)
    {
        glm::vec3 n = glm::normalize(normals[i]);
        mesh.Vertices[i].Normal = { n.x, n.y, n.z };
    }

    // --- 4. Finalise and upload ---
    mesh.DiffuseColor = { 0.35f, 0.55f, 0.25f };  // grassy green
    mesh.CalculateBounds();
    mesh.Upload();

    // --- 5. Free old GPU resources and register in MeshManager ---
    if (entity.MeshHandle != 0)
    {
        Mesh* old = MeshManager::Instance().GetMesh(entity.MeshHandle);
        if (old)
        {
            if (old->VAO) { glDeleteVertexArrays(1, &old->VAO); old->VAO = 0; }
            if (old->VBO) { glDeleteBuffers(1, &old->VBO);      old->VBO = 0; }
            if (old->EBO) { glDeleteBuffers(1, &old->EBO);      old->EBO = 0; }
        }
        MeshManager::Instance().Release(entity.MeshHandle);
        entity.MeshHandle = 0;
    }

    const std::string meshKey = "[terrain]" + entity.name;
    entity.MeshHandle = MeshManager::Instance().RegisterMesh(meshKey, std::move(mesh));
    entity.MeshPath   = "[terrain]";
}

// ---------------------------------------------------------------------------
// TerrainSystem::SampleHeight
// ---------------------------------------------------------------------------

float TerrainSystem::SampleHeight(const Entity& terrain, float worldX, float worldZ)
{
    if (!terrain.IsTerrain || terrain.TerrainHeightData.empty())
        return -FLT_MAX;

    const float halfX = terrain.Transform.Scale.x * 0.5f;
    const float halfZ = terrain.Transform.Scale.z * 0.5f;
    const float minX  = terrain.Transform.Position.x - halfX;
    const float maxX  = terrain.Transform.Position.x + halfX;
    const float minZ  = terrain.Transform.Position.z - halfZ;
    const float maxZ  = terrain.Transform.Position.z + halfZ;

    if (worldX < minX || worldX > maxX || worldZ < minZ || worldZ > maxZ)
        return -FLT_MAX;

    const float u = (worldX - minX) / terrain.Transform.Scale.x;
    const float v = (worldZ - minZ) / terrain.Transform.Scale.z;

    const float h = SampleHeightBilinear(terrain.TerrainHeightData,
                                          terrain.TerrainHeightDataWidth,
                                          terrain.TerrainHeightDataDepth, u, v);

    return terrain.Transform.Position.y + h * terrain.Transform.Scale.y;
}
