#pragma once
#include "Renderable.h"

class Entity : public Renderable
{
public:
	virtual ~Entity() = default;

	const std::string& GetName() const { return name; }
	void SetName(const std::string& n) { name = n; }

protected:
	std::string name = "Entity";
};

