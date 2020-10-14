#pragma once
#include "IComponentManager.h"
#include "../Component/TransformComponent.h"

class ITransformComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ITransformComponentManager);
	virtual TransformComponent* Get(std::size_t index) = 0;
	virtual const std::vector<TransformComponent*>& GetAllComponents() = 0;
	virtual const TransformComponent* GetRootTransformComponent() const = 0;
};