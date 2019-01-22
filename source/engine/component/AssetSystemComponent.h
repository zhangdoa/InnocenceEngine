#pragma once
#include "../common/InnoType.h"
#include "../component/MeshDataComponent.h"
#include "../component/MaterialDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/VisibleComponent.h"

#include "../common/InnoConcurrency.h"

struct AssetMetadata
{
	std::string fullPath;
	std::string fileName;
	std::string extension;
	FileExplorerIconType iconType;
};

struct DirectoryMetadata
{
	unsigned int depth = 0;
	std::string directoryName = "root";
	DirectoryMetadata* parentDirectory = 0;
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

	std::unordered_map<EntityID, MeshDataComponent*> m_meshMap;
	std::unordered_map<EntityID, MaterialDataComponent*> m_materialMap;
	std::unordered_map<EntityID, TextureDataComponent*> m_textureMap;

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

	TextureDataComponent* m_iconTemplate_DirectionalLight;
	TextureDataComponent* m_iconTemplate_PointLight;
	TextureDataComponent* m_iconTemplate_SphereLight;

    const std::string m_textureRelativePath = std::string{"..//res//textures//"};
    const std::string m_modelRelativePath = std::string{"..//res//models//"};

	DirectoryMetadata m_rootDirectoryMetadata;
private:
	AssetSystemComponent() {};
};