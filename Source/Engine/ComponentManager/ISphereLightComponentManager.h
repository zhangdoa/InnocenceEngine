#pragma once
#include "IComponentManager.h"
#include "../Component/SphereLightComponent.h"

class ISphereLightComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISphereLightComponentManager);
	virtual const std::vector<SphereLightComponent*>& GetAllComponents() = 0;
};