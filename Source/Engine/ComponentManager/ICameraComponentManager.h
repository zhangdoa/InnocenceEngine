#pragma once
#include "IComponentManager.h"
#include "../Component/CameraComponent.h"

INNO_INTERFACE ICameraComponentManager : INNO_IMPLEMENT IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ICameraComponentManager);
	virtual const std::vector<CameraComponent*>& GetAllComponents() = 0;
};