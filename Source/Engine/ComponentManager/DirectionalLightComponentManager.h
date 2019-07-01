#pragma once
#include "IDirectionalLightComponentManager.h"

INNO_CONCRETE InnoDirectionalLightComponentManager : INNO_IMPLEMENT IDirectionalLightComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoDirectionalLightComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destory(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<DirectionalLightComponent*>& GetAllComponents() override;
};