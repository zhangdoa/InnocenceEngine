#pragma once
#include "IComponentManager.h"
#include "../Component/SpotLightComponent.h"

INNO_INTERFACE ISpotLightComponentManager : INNO_IMPLEMENT IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISpotLightComponentManager);
	virtual const std::vector<SpotLightComponent*>& GetAllComponents() = 0;
};