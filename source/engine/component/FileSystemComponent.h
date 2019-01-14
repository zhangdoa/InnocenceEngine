#pragma once
#include "../common/InnoType.h"
#include "../component/MeshDataComponent.h"
#include "../component/MaterialDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/VisibleComponent.h"

#include "../common/InnoConcurrency.h"

class FileSystemComponent
{
public:
	~FileSystemComponent() {};

	static FileSystemComponent& get()
	{
		static FileSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

	std::unordered_map<std::string, ModelMap> m_loadedModelMap;
	std::unordered_map<std::string, ModelPair> m_loadedModelPair;
	std::unordered_map<std::string, TextureDataComponent*> m_loadedTexture;

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMeshComponents;
	ThreadSafeQueue<TextureDataComponent*> m_uninitializedTextureComponents;

private:
	FileSystemComponent() {};
};