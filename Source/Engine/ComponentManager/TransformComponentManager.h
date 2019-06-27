#pragma once
#include "ITransformComponentManager.h"

INNO_CONCRETE TransformComponentManager : INNO_IMPLEMENT ITransformComponentManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(TransformComponentManager);

	bool Setup() override;
	bool Initialize() override;
	bool Simulate() override;
	bool Terminate() override;
	InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) override;
	void Destory(InnoComponent* component) override;
};