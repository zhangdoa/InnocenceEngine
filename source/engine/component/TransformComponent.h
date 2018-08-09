#pragma once
#include "BaseComponent.h"
#include "component/InnoMath.h"

class TransformComponent : public BaseComponent
{
public:
	TransformComponent() {};
	~TransformComponent() {};

	Transform m_transform;
};

