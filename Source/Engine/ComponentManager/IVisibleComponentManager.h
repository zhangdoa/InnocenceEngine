#pragma once
#include "IComponentManager.h"
#include "../Component/VisibleComponent.h"

class IVisibleComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IVisibleComponentManager);
	virtual VisibleComponent* Get(std::size_t index) = 0;
	virtual const std::vector<VisibleComponent*>& GetAllComponents() = 0;
};