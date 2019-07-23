#pragma once
#include "ICameraComponentManager.h"

class InnoCameraComponentManager : public ICameraComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoCameraComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destory(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<CameraComponent*>& GetAllComponents() override;
};