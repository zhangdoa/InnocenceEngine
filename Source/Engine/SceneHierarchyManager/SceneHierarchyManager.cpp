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

bool InnoSceneHierarchyManager::Register(const InnoEntity * entity)
{
	return true;
}

bool InnoSceneHierarchyManager::Unregister(const InnoEntity * entity)
{
	return true;
}

bool Register(const InnoComponent * component)
{
	return true;
}

bool Unregister(const InnoComponent * component)
{
	return true;
}