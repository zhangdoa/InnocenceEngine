#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"
#include "../Common/InnoObject.h"

using SceneHierarchyMap = std::unordered_map<InnoEntity*, std::set<InnoComponent*>>;

class ISceneHierarchyManager
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(ISceneHierarchyManager);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool Terminate() = 0;
	virtual const SceneHierarchyMap& GetSceneHierarchyMap() = 0;
};