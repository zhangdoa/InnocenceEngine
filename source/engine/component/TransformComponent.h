#pragma once
#include "BaseComponent.h"
#include "entity/InnoMath.h"

class TransformComponent : public BaseComponent
{
public:
	TransformComponent() {};
	~TransformComponent() {};

	void setup() override;
	void initialize() override;
	void shutdown() override;

	Transform m_transform;
	Transform* m_parentTransform;
};

