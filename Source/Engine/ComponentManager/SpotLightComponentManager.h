#pragma once
#include "ISpotLightComponentManager.h"

class InnoSpotLightComponentManager : public ISpotLightComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoSpotLightComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destroy(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<SpotLightComponent*>& GetAllComponents() override;
};