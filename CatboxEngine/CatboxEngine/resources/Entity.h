#pragma once
#include "Transform.h"
#include "../graphics/Mesh.h"
#include "../graphics/Material.h"
#include <string>

class Entity
{
public:
    std::string name;

    Transform Transform;
    Mesh Mesh;
    //Material Material;
};
