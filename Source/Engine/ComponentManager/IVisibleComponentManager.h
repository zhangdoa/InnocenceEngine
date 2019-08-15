#pragma once
#include "IComponentManager.h"
#include "../Component/VisibleComponent.h"

class IVisibleComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IVisibleComponentManager);
	virtual const std::vector<VisibleComponent*>& GetAllComponents() = 0;
	virtual void LoadAssetsForComponents(bool AsyncLoad = true) = 0;
};