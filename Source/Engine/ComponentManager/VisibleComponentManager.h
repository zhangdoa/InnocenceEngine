#pragma once
#include "IVisibleComponentManager.h"

INNO_CONCRETE InnoVisibleComponentManager : INNO_IMPLEMENT IVisibleComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoVisibleComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destory(InnoComponent* component) override;
	InnoComponent* Find(const InnoEntity* parentEntity) override;

	const std::vector<VisibleComponent*>& GetAllComponents() override;
	void LoadAssetsForComponents() override;
};