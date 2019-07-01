#pragma once
#include "ISpotLightComponentManager.h"

INNO_CONCRETE InnoSpotLightComponentManager : INNO_IMPLEMENT ISpotLightComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoSpotLightComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destory(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<SpotLightComponent*>& GetAllComponents() override;
};