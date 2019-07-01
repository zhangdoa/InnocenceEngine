#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Common/InnoEntity.h"
#include "../Common/InnoComponent.h"

using SceneHierarchyMap = std::unordered_map<InnoEntity*, std::set<InnoComponent*>>;

INNO_INTERFACE ISceneHierarchyManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISceneHierarchyManager);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Terminate() = 0;
	virtual const SceneHierarchyMap& GetSceneHierarchyMap() = 0;
};