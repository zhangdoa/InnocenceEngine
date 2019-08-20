#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Common/InnoComponent.h"

class IComponentManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IComponentManager);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Simulate() = 0;
	virtual bool Terminate() = 0;
	virtual InnoComponent* Spawn(const InnoEntity* parentEntity, ObjectSource objectSource, ObjectUsage objectUsage) = 0;
	virtual void Destroy(InnoComponent* component) = 0;
	virtual InnoComponent* Find(const InnoEntity* parentEntity) = 0;
};