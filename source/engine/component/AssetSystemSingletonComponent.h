#pragma once
#include "BaseComponent.h"

#include "../component/GLMeshDataComponent.h"
#include "../component/GLTextureDataComponent.h"

#include "../component/DXMeshDataComponent.h"
#include "../component/DXTextureDataComponent.h"

#include "../common/InnoConcurrency.h"
class AssetSystemSingletonComponent : public BaseComponent
{
public:
	~AssetSystemSingletonComponent() {};

	static AssetSystemSingletonComponent& getInstance()
	{
		static AssetSystemSingletonComponent instance;
		return instance;
	}

    std::unordered_map<std::string, int> m_supportedTextureType = { {"png", 0} };
    std::unordered_map<std::string, int> m_supportedModelType = { {"obj", 0}, {"innoModel", 0} };
    std::unordered_map<std::string, int> m_supportedShaderType = { {"sf", 0} };

	std::unordered_map<EntityID, MeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, TextureDataComponent*> m_textureMap;

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMeshComponents;
	ThreadSafeQueue<TextureDataComponent*> m_uninitializedTextureComponents;

	EntityID m_UnitLineTemplate;
	EntityID m_UnitQuadTemplate;
	EntityID m_UnitCubeTemplate;
	EntityID m_UnitSphereTemplate;

	EntityID m_basicNormalTemplate;
	EntityID m_basicAlbedoTemplate;
	EntityID m_basicMetallicTemplate;
	EntityID m_basicRoughnessTemplate;
	EntityID m_basicAOTemplate;

	std::unordered_map<std::string, modelMap> m_loadedModelMap;
	std::unordered_map<std::string, texturePair> m_loadedTextureMap;

    const std::string m_textureRelativePath = std::string{"..//res//textures//"};
    const std::string m_modelRelativePath = std::string{"..//res//models//"};
    const std::string m_shaderRelativePath = std::string{"..//res//shaders//"};

	std::vector<InnoFuture<void>> m_asyncTaskVector;
private:
	AssetSystemSingletonComponent() {};
};
