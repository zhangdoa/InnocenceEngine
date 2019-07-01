#pragma once
#include "ISceneHierarchyManager.h"

INNO_CONCRETE InnoSceneHierarchyManager : INNO_IMPLEMENT ISceneHierarchyManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoSceneHierarchyManager);

	bool Setup() override;
	bool Initialize() override;
	bool Terminate() override;
	const SceneHierarchyMap& GetSceneHierarchyMap() override;
};