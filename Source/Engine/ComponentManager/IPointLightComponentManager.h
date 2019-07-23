#pragma once
#include "IComponentManager.h"
#include "../Component/PointLightComponent.h"

class IPointLightComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IPointLightComponentManager);
	virtual const std::vector<PointLightComponent*>& GetAllComponents() = 0;
};