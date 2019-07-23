#pragma once
#include "ISceneHierarchyManager.h"

class InnoSceneHierarchyManager : public ISceneHierarchyManager
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoSceneHierarchyManager);

	bool Setup() override;
	bool Initialize() override;
	bool Terminate() override;
	const SceneHierarchyMap& GetSceneHierarchyMap() override;
};