#pragma once
#include "IComponentManager.h"
#include "../Component/LightComponent.h"

class ILightComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ILightComponentManager);
	virtual const std::vector<LightComponent*>& GetAllComponents() = 0;
};