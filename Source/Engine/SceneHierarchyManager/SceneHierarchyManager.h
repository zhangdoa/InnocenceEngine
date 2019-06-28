#pragma once
#include "ISceneHierarchyManager.h"

INNO_CONCRETE InnoSceneHierarchyManager : INNO_IMPLEMENT ISceneHierarchyManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoSceneHierarchyManager);

	bool Setup() override;
	bool Initialize() override;
	bool Terminate() override;
	bool Register(const InnoEntity* entity) override;
	bool Unregister(const InnoEntity* entity) override;
	bool Register(const InnoComponent* component) override;
	bool Unregister(const InnoComponent* component) override;
};