#pragma once
#include "IComponentManager.h"
#include "../Component/DirectionalLightComponent.h"

class IDirectionalLightComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IDirectionalLightComponentManager);
	virtual const std::vector<DirectionalLightComponent*>& GetAllComponents() = 0;
	virtual const std::vector<AABB>& GetSplitAABB() = 0;
	virtual const std::vector<mat4>& GetProjectionMatrices() = 0;
};