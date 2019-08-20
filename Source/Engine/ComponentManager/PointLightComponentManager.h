#pragma once
#include "IPointLightComponentManager.h"

class InnoPointLightComponentManager : public IPointLightComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoPointLightComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destroy(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<PointLightComponent*>& GetAllComponents() override;
};