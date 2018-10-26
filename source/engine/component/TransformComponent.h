#pragma once
#include "BaseComponent.h"

class TransformComponent : public BaseComponent
{
public:
	TransformComponent() {};
	~TransformComponent() {};

	Transform m_currentTransform;
	Transform m_previousTransform;
};

