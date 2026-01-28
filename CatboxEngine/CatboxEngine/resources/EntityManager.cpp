#include "EntityManager.h"
#include "../graphics/MeshManager.h"

int EntityManager::AddEntity(const Entity& e, bool useSharedCube)
{
    m_entities.push_back(e);
    int idx = (int)m_entities.size() - 1;

    Entity& ent = m_entities[idx];
    // if caller provided a MeshHandle in e.MeshHandle use that, otherwise create or assign a cube
    if (ent.MeshHandle == 0)
    {
        if (useSharedCube)
        {
            MeshHandle h = MeshManager::Instance().GetSharedCubeHandle();
            ent.MeshHandle = h;
        }
        else
        {
            // create a unique cube for this entity
            MeshHandle h = MeshManager::Instance().GetSharedCubeHandle();
            ent.MeshHandle = h;
        }
    }

    return idx;
}

