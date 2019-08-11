#pragma once
#include "IComponentManager.h"
#include "../Component/CameraComponent.h"

class ICameraComponentManager : public IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ICameraComponentManager);
	virtual const std::vector<CameraComponent*>& GetAllComponents() = 0;
	virtual CameraComponent* GetMainCamera() = 0;
};