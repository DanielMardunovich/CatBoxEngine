#include "EntityManager.h"
#include "../graphics/MeshManager.h"

int EntityManager::AddEntity(const Entity& e, bool useSharedCube)
{
    m_entities.push_back(e);
    int idx = (int)m_entities.size() - 1;

    Entity& ent = m_entities[idx];
    if (ent.Mesh.VAO == 0)
    {
        if (useSharedCube)
        {
            MeshHandle h = MeshManager::Instance().GetSharedCubeHandle();
            Mesh* shared = MeshManager::Instance().GetMesh(h);
            if (shared) ent.Mesh = *shared;
            // keep handle alive by not releasing here; Entity owns its copy
        }
        else
        {
            Mesh m = CreateCubeMesh();
            m.Upload();
            ent.Mesh = m;
        }
    }

    return idx;
}

