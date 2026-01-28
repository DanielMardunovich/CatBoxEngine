#pragma once
#include "Transform.h"
#include "../graphics/Mesh.h"
#include "../graphics/MeshManager.h"
#include "../graphics/Material.h"
#include <string>

class Entity
{
public:
    std::string name;

    Transform Transform;
    // store mesh by handle (managed by MeshManager)
    MeshHandle MeshHandle = 0;
    //Material Material;
};
