#include "SceneHierarchyManager.h"

#include "../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

bool InnoSceneHierarchyManager::Setup()
{
	return true;
}

bool InnoSceneHierarchyManager::Initialize()
{
	return true;
}

bool InnoSceneHierarchyManager::Terminate()
{
	return true;
}

const SceneHierarchyMap & InnoSceneHierarchyManager::GetSceneHierarchyMap()
{
	return SceneHierarchyMap();
}