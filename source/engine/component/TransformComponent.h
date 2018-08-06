#pragma once
#include "BaseComponent.h"
#include "entity/InnoMath.h"

class TransformComponent : public BaseComponent
{
public:
	TransformComponent() {};
	~TransformComponent() {};

	Transform m_transform;
};

