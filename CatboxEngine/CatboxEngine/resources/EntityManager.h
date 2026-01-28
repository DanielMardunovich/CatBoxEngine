#pragma once
#include "Entity.h"
#include <vector>
#include "../graphics/Mesh.h"

class EntityManager
{
public:
    void Add(const Entity& e) { m_entities.push_back(e); }
    // add entity and ensure it has a mesh (cube if none). returns index
    int AddEntity(const Entity& e, bool useSharedCube);
    const std::vector<Entity>& GetAll() const { return m_entities; }
    std::vector<Entity>& GetAll() { return m_entities; }
    void RemoveAt(size_t idx) { if (idx < m_entities.size()) m_entities.erase(m_entities.begin() + idx); }
    void Clear() { m_entities.clear(); }
    size_t Size() const { return m_entities.size(); }

private:
    std::vector<Entity> m_entities;
    // mesh manager
    Mesh sharedCube; // kept for compatibility
    bool sharedInitialized = false;
    // mesh manager is accessed via MeshManager::Instance()
    
    // create a cube mesh
    static Mesh CreateCubeMesh()
    {
        Mesh mesh;

        mesh.Vertices = {
            // Front (+Z)
            {{-0.5f, -0.5f,  0.5f}, {0, 0, 1}, {0, 0, 0}},
            {{ 0.5f, -0.5f,  0.5f}, {0, 0, 1}, {1, 0, 0}},
            {{ 0.5f,  0.5f,  0.5f}, {0, 0, 1}, {1, 1, 0}},
            {{-0.5f,  0.5f,  0.5f}, {0, 0, 1}, {0, 1, 0}},

            // Back (-Z)
            {{ 0.5f, -0.5f, -0.5f}, {0, 0, -1}, {0, 0, 0}},
            {{-0.5f, -0.5f, -0.5f}, {0, 0, -1}, {1, 0, 0}},
            {{-0.5f,  0.5f, -0.5f}, {0, 0, -1}, {1, 1, 0}},
            {{ 0.5f,  0.5f, -0.5f}, {0, 0, -1}, {0, 1, 0}},

            // Left (-X)
            {{-0.5f, -0.5f, -0.5f}, {-1, 0, 0}, {0, 0, 0}},
            {{-0.5f, -0.5f,  0.5f}, {-1, 0, 0}, {1, 0, 0}},
            {{-0.5f,  0.5f,  0.5f}, {-1, 0, 0}, {1, 1, 0}},
            {{-0.5f,  0.5f, -0.5f}, {-1, 0, 0}, {0, 1, 0}},

            // Right (+X)
            {{ 0.5f, -0.5f,  0.5f}, {1, 0, 0}, {0, 0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, {1, 0, 0}, {1, 0, 0}},
            {{ 0.5f,  0.5f, -0.5f}, {1, 0, 0}, {1, 1, 0}},
            {{ 0.5f,  0.5f,  0.5f}, {1, 0, 0}, {0, 1, 0}},

            // Top (+Y)
            {{-0.5f,  0.5f,  0.5f}, {0, 1, 0}, {0, 0, 0}},
            {{ 0.5f,  0.5f,  0.5f}, {0, 1, 0}, {1, 0, 0}},
            {{ 0.5f,  0.5f, -0.5f}, {0, 1, 0}, {1, 1, 0}},
            {{-0.5f,  0.5f, -0.5f}, {0, 1, 0}, {0, 1, 0}},

            // Bottom (-Y)
            {{-0.5f, -0.5f, -0.5f}, {0, -1, 0}, {0, 0, 0}},
            {{ 0.5f, -0.5f, -0.5f}, {0, -1, 0}, {1, 0, 0}},
            {{ 0.5f, -0.5f,  0.5f}, {0, -1, 0}, {1, 1, 0}},
            {{-0.5f, -0.5f,  0.5f}, {0, -1, 0}, {0, 1, 0}},
        };


        mesh.Indices = {
            // Front face
            0,1,2, 2,3,0,
            // Back face
            4,5,6, 6,7,4,
            // Left face
            8,9,10, 10,11,8,
            // Right face
            12,13,14, 14,15,12,
            // Top face
            16,17,18, 18,19,16,
            // Bottom face
            20,21,22, 22,23,20
        };

        return mesh;
    }
};

