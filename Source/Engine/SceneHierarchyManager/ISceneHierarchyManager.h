#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Common/InnoEntity.h"
#include "../Common/InnoComponent.h"
INNO_INTERFACE ISceneHierarchyManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISceneHierarchyManager);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Terminate() = 0;
	virtual bool Register(const InnoEntity* entity) = 0;
	virtual bool Unregister(const InnoEntity* entity) = 0;
	virtual bool Register(const InnoComponent* component) = 0;
	virtual bool Unregister(const InnoComponent* component) = 0;
};