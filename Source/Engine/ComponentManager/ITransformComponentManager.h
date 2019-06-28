#pragma once
#include "IComponentManager.h"
#include "../Component/TransformComponent.h"

INNO_INTERFACE ITransformComponentManager : INNO_IMPLEMENT IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITransformComponentManager);
	virtual const std::vector<TransformComponent*>& GetAllComponents() = 0;
	virtual void SaveCurrentFrameTransform() = 0;
	virtual const TransformComponent* GetRootTransformComponent() const = 0;
};