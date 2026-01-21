#pragma once
#include "Shader.h"
class Renderable
{
public:
	virtual ~Renderable() = default;
	virtual void Render(Shader& aShader) = 0;
};

