#pragma once
#include "IVisibleComponentManager.h"

class InnoVisibleComponentManager : public IVisibleComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoVisibleComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destroy(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<VisibleComponent*>& GetAllComponents() override;
	void LoadAssetsForComponents(bool AsyncLoad) override;
};