#pragma once
#include "IComponentManager.h"
#include "../Component/DirectionalLightComponent.h"

INNO_INTERFACE IDirectionalLightComponentManager : INNO_IMPLEMENT IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IDirectionalLightComponentManager);
	virtual const std::vector<DirectionalLightComponent*>& GetAllComponents() = 0;
};