#pragma once
#include "ISphereLightComponentManager.h"

INNO_CONCRETE InnoSphereLightComponentManager : INNO_IMPLEMENT ISphereLightComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoSphereLightComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destory(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<SphereLightComponent*>& GetAllComponents() override;
};