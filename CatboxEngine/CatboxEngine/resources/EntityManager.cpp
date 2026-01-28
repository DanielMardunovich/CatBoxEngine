#include "EntityManager.h"

int EntityManager::AddEntity(const Entity& e, bool useSharedCube)
{
    m_entities.push_back(e);
    int idx = (int)m_entities.size() - 1;

    Entity& ent = m_entities[idx];
    if (ent.Mesh.VAO == 0)
    {
        if (useSharedCube)
        {
            if (!sharedInitialized)
            {
                sharedCube = CreateCubeMesh();
                sharedCube.Upload();
                sharedInitialized = true;
            }
            ent.Mesh = sharedCube;
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

