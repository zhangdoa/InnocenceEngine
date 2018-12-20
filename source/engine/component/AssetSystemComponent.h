#pragma once
#include "../common/InnoType.h"
#include "../component/MeshDataComponent.h"
#include "../component/MaterialDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/VisibleComponent.h"

#include "../common/InnoConcurrency.h"

struct AssetMetadata
{
	std::string fileName;
	std::string extension;
	IconType iconType;
};

struct DirectoryMetadata
{
	unsigned int depth = 0;
	std::string directoryName = "root";
	DirectoryMetadata* parentDirectory;
	std::vector<DirectoryMetadata> childrenDirectories;
	std::vector<AssetMetadata> childrenAssets;
};

class AssetSystemComponent
{
public:
	~AssetSystemComponent() {};

	static AssetSystemComponent& get()
	{
		static AssetSystemComponent instance;
		return instance;
	}

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_parentEntity;

    std::unordered_map<std::string, int> m_supportedTextureType = { {"png", 0} };
    std::unordered_map<std::string, int> m_supportedModelType = { {"obj", 0}, {"innoModel", 0} };
    std::unordered_map<std::string, int> m_supportedShaderType = { {"sf", 0} };

	std::unordered_map<EntityID, MeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, MaterialDataComponent*> m_materialMap;
	std::unordered_map<EntityID, TextureDataComponent*> m_textureMap;

	ThreadSafeQueue<MeshDataComponent*> m_uninitializedMeshComponents;
	ThreadSafeQueue<TextureDataComponent*> m_uninitializedTextureComponents;

	MeshDataComponent* m_UnitLineTemplate;
	MeshDataComponent* m_UnitQuadTemplate;
	MeshDataComponent* m_UnitCubeTemplate;
	MeshDataComponent* m_UnitSphereTemplate;
	MeshDataComponent* m_Terrain;

	TextureDataComponent* m_basicNormalTemplate;
	TextureDataComponent* m_basicAlbedoTemplate;
	TextureDataComponent* m_basicMetallicTemplate;
	TextureDataComponent* m_basicRoughnessTemplate;
	TextureDataComponent* m_basicAOTemplate;

	TextureDataComponent* m_iconTemplate_OBJ;
	TextureDataComponent* m_iconTemplate_PNG;
	TextureDataComponent* m_iconTemplate_SHADER;
	TextureDataComponent* m_iconTemplate_UNKNOWN;

	std::unordered_map<std::string, ModelMap> m_loadedModelMap;
	std::unordered_map<std::string, TextureDataComponent*> m_loadedTextureMap;

    const std::string m_textureRelativePath = std::string{"..//res//textures//"};
    const std::string m_modelRelativePath = std::string{"..//res//models//"};
    const std::string m_shaderRelativePath = std::string{"..//res//shaders//"};

	DirectoryMetadata m_rootDirectoryMetadata;

	std::vector<InnoFuture<void>> m_asyncTaskVector;
private:
	AssetSystemComponent() {};
};