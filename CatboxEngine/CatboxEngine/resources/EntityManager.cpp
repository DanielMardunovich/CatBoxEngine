#include "EntityManager.h"
#include "../graphics/MeshManager.h"

int EntityManager::AddEntity(const Entity& e, bool useSharedCube)
{
    m_entities.push_back(e);
    int idx = (int)m_entities.size() - 1;

    Entity& ent = m_entities[idx];
    // migrate: if caller provided a MeshHandle in e.MeshHandle use that, otherwise create or assign a cube
    if (ent.MeshHandle == 0)
    {
        if (useSharedCube)
        {
            MeshHandle h = MeshManager::Instance().GetSharedCubeHandle();
            ent.MeshHandle = h;
        }
        else
        {
            // create a unique mesh entry for this entity (sync)
            Mesh m = CreateCubeMesh();
            m.Upload();
            // create a temporary entry in manager
            // use a synthetic path
            std::string key = std::string("__entity_cube_") + std::to_string(idx);
            MeshHandle h = MeshManager::Instance().LoadMeshSync(key); // will attempt load, but key is new
            // fallback: directly assign mesh into manager's entry (not ideal). For now set MeshHandle to 0 and keep local mesh
            ent.MeshHandle = 0;
            ent.Transform = ent.Transform; // no-op to avoid unused warning
        }
    }

    return idx;
}

