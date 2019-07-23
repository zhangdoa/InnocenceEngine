#pragma once
#include "IComponentManager.h"
#include "../Component/SpotLightComponent.h"

class ISpotLightComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISpotLightComponentManager);
	virtual const std::vector<SpotLightComponent*>& GetAllComponents() = 0;
};