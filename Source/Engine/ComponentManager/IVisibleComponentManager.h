#pragma once
#include "IComponentManager.h"
#include "../Component/VisibleComponent.h"

INNO_INTERFACE IVisibleComponentManager : INNO_IMPLEMENT IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IVisibleComponentManager);
	virtual const std::vector<VisibleComponent*>& GetAllComponents() = 0;
	virtual void LoadAssetsForComponents() = 0;
};