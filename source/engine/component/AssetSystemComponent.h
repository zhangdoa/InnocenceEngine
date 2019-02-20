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

	MeshDataComponent* m_UnitLineMDC;
	MeshDataComponent* m_UnitQuadMDC;
	MeshDataComponent* m_UnitCubeMDC;
	MeshDataComponent* m_UnitSphereMDC;
	MeshDataComponent* m_TerrainMDC;

	TextureDataComponent* m_basicNormalTDC;
	TextureDataComponent* m_basicAlbedoTDC;
	TextureDataComponent* m_basicMetallicTDC;
	TextureDataComponent* m_basicRoughnessTDC;
	TextureDataComponent* m_basicAOTDC;

	TextureDataComponent* m_iconTemplate_OBJ;
	TextureDataComponent* m_iconTemplate_PNG;
	TextureDataComponent* m_iconTemplate_SHADER;
	TextureDataComponent* m_iconTemplate_UNKNOWN;

	TextureDataComponent* m_iconTemplate_DirectionalLight;
	TextureDataComponent* m_iconTemplate_PointLight;
	TextureDataComponent* m_iconTemplate_SphereLight;

	DirectoryMetadata m_rootDirectoryMetadata;
private:
	AssetSystemComponent() {};
};