#include "MTRenderingSystem.h"
#include "../../component/MTMeshDataComponent.h"
#include "../../component/MaterialDataComponent.h"
#include "../../component/MTTextureDataComponent.h"

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE MTRenderingSystemNS
{
	bool setup();
	bool initialize();

	void loadDefaultAssets();
	bool update();
	bool render();
	bool resize();
	bool terminate();

	MTMeshDataComponent* addMTMeshDataComponent();
	MaterialDataComponent* addMaterialDataComponent();
	MTTextureDataComponent* addMTTextureDataComponent();

	MTMeshDataComponent* getMTMeshDataComponent(EntityID meshID);
	MTTextureDataComponent* getMTTextureDataComponent(EntityID textureID);

	MTMeshDataComponent* getMTMeshDataComponent(MeshShapeType MeshShapeType);
	MTTextureDataComponent* getMTTextureDataComponent(TextureUsageType TextureUsageType);
	MTTextureDataComponent* getMTTextureDataComponent(FileExplorerIconType iconType);
	MTTextureDataComponent* getMTTextureDataComponent(WorldEditorIconType iconType);

	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
	EntityID m_entityID;

	MTRenderingSystemBridge* m_bridge;

	ThreadSafeUnorderedMap<EntityID, MTMeshDataComponent*> m_meshMap;
	ThreadSafeUnorderedMap<EntityID, MaterialDataComponent*> m_materialMap;
	ThreadSafeUnorderedMap<EntityID, MTTextureDataComponent*> m_textureMap;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<MTMeshDataComponent*> m_uninitializedMDC;
	ThreadSafeQueue<MTTextureDataComponent*> m_uninitializedTDC;

	MTTextureDataComponent* m_iconTemplate_OBJ;
	MTTextureDataComponent* m_iconTemplate_PNG;
	MTTextureDataComponent* m_iconTemplate_SHADER;
	MTTextureDataComponent* m_iconTemplate_UNKNOWN;

	MTTextureDataComponent* m_iconTemplate_DirectionalLight;
	MTTextureDataComponent* m_iconTemplate_PointLight;
	MTTextureDataComponent* m_iconTemplate_SphereLight;

	MTMeshDataComponent* m_unitLineMDC;
	MTMeshDataComponent* m_unitQuadMDC;
	MTMeshDataComponent* m_unitCubeMDC;
	MTMeshDataComponent* m_unitSphereMDC;
	MTMeshDataComponent* m_terrainMDC;

	MTTextureDataComponent* m_basicNormalTDC;
	MTTextureDataComponent* m_basicAlbedoTDC;
	MTTextureDataComponent* m_basicMetallicTDC;
	MTTextureDataComponent* m_basicRoughnessTDC;
	MTTextureDataComponent* m_basicAOTDC;
}

bool MTRenderingSystemNS::setup()
{
	m_entityID = InnoMath::createEntityID();

	bool result = MTRenderingSystemNS::m_bridge->setup();

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem setup finished.");
	return true;
}

bool MTRenderingSystemNS::initialize()
{
	m_MeshDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MTMeshDataComponent), 16384);
	m_MaterialDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MaterialDataComponent), 32768);
	m_TextureDataComponentPool = g_pCoreSystem->getMemorySystem()->allocateMemoryPool(sizeof(MTTextureDataComponent), 32768);

	bool result = MTRenderingSystemNS::m_bridge->initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem has been initialized.");
	return true;
}

bool MTRenderingSystemNS::update()
{
	bool result = MTRenderingSystemNS::m_bridge->update();

	return true;
}

bool MTRenderingSystemNS::render()
{
	return true;
}

bool MTRenderingSystemNS::terminate()
{
	bool result = m_bridge->terminate();

	MTRenderingSystemNS::m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem has been terminated.");

	return true;
}

void MTRenderingSystemNS::loadDefaultAssets()
{
	auto l_basicNormalTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_basicAlbedoTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	auto l_basicMetallicTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	auto l_basicRoughnessTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	auto l_basicAOTDC = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	auto l_iconTemplate_OBJ = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PNG = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SHADER = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_UNKNOWN = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	auto l_iconTemplate_DirectionalLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PointLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SphereLight = g_pCoreSystem->getAssetSystem()->loadTexture("res//textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
}

MTMeshDataComponent* MTRenderingSystemNS::addMTMeshDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(MTMeshDataComponent));
	auto l_MDC = new(l_rawPtr)MTMeshDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_meshMap = &m_meshMap;
	l_meshMap->emplace(std::pair<EntityID, MTMeshDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

MaterialDataComponent* MTRenderingSystemNS::addMaterialDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(MaterialDataComponent));
	auto l_MDC = new(l_rawPtr)MaterialDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_MDC->m_parentEntity = l_parentEntity;
	auto l_materialMap = &m_materialMap;
	l_materialMap->emplace(std::pair<EntityID, MaterialDataComponent*>(l_parentEntity, l_MDC));
	return l_MDC;
}

MTTextureDataComponent* MTRenderingSystemNS::addMTTextureDataComponent()
{
	auto l_rawPtr = g_pCoreSystem->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(MTTextureDataComponent));
	auto l_TDC = new(l_rawPtr)MTTextureDataComponent();
	auto l_parentEntity = InnoMath::createEntityID();
	l_TDC->m_parentEntity = l_parentEntity;
	auto l_textureMap = &m_textureMap;
	l_textureMap->emplace(std::pair<EntityID, MTTextureDataComponent*>(l_parentEntity, l_TDC));
	return l_TDC;
}

MTMeshDataComponent* MTRenderingSystemNS::getMTMeshDataComponent(EntityID EntityID)
{
	auto l_result = MTRenderingSystemNS::m_meshMap.find(EntityID);
	if (l_result != MTRenderingSystemNS::m_meshMap.end())
	{
		return l_result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't find MeshDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

MTTextureDataComponent * MTRenderingSystemNS::getMTTextureDataComponent(EntityID EntityID)
{
	auto l_result = MTRenderingSystemNS::m_textureMap.find(EntityID);
	if (l_result != MTRenderingSystemNS::m_textureMap.end())
	{
		return l_result->second;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't find TextureDataComponent by EntityID: " + EntityID + " !");
		return nullptr;
	}
}

MTMeshDataComponent* MTRenderingSystemNS::getMTMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return MTRenderingSystemNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return MTRenderingSystemNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return MTRenderingSystemNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return MTRenderingSystemNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return MTRenderingSystemNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: wrong MeshShapeType passed to MTRenderingSystem::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

MTTextureDataComponent * MTRenderingSystemNS::getMTTextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return MTRenderingSystemNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return MTRenderingSystemNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return MTRenderingSystemNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return MTRenderingSystemNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return MTRenderingSystemNS::m_basicAOTDC; break;
	case TextureUsageType::COLOR_ATTACHMENT:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

MTTextureDataComponent * MTRenderingSystemNS::getMTTextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return MTRenderingSystemNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return MTRenderingSystemNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return MTRenderingSystemNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return MTRenderingSystemNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

MTTextureDataComponent * MTRenderingSystemNS::getMTTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return MTRenderingSystemNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return MTRenderingSystemNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return MTRenderingSystemNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool MTRenderingSystemNS::resize()
{
	return true;
}

bool MTRenderingSystem::setup()
{
	return MTRenderingSystemNS::setup();
}

bool MTRenderingSystem::initialize()
{
	return MTRenderingSystemNS::initialize();
}

bool MTRenderingSystem::update()
{
	return MTRenderingSystemNS::update();
}

bool MTRenderingSystem::render()
{
	return MTRenderingSystemNS::render();
}

bool MTRenderingSystem::terminate()
{
	return MTRenderingSystemNS::terminate();
}

ObjectStatus MTRenderingSystem::getStatus()
{
	return MTRenderingSystemNS::m_objectStatus;
}

MeshDataComponent * MTRenderingSystem::addMeshDataComponent()
{
	return MTRenderingSystemNS::addMTMeshDataComponent();
}

MaterialDataComponent * MTRenderingSystem::addMaterialDataComponent()
{
	return MTRenderingSystemNS::addMaterialDataComponent();
}

TextureDataComponent * MTRenderingSystem::addTextureDataComponent()
{
	return MTRenderingSystemNS::addMTTextureDataComponent();
}

MeshDataComponent * MTRenderingSystem::getMeshDataComponent(EntityID meshID)
{
	return MTRenderingSystemNS::getMTMeshDataComponent(meshID);
}

TextureDataComponent * MTRenderingSystem::getTextureDataComponent(EntityID textureID)
{
	return MTRenderingSystemNS::getMTTextureDataComponent(textureID);
}

MeshDataComponent * MTRenderingSystem::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return MTRenderingSystemNS::getMTMeshDataComponent(MeshShapeType);
}

TextureDataComponent * MTRenderingSystem::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return MTRenderingSystemNS::getMTTextureDataComponent(TextureUsageType);
}

TextureDataComponent * MTRenderingSystem::getTextureDataComponent(FileExplorerIconType iconType)
{
	return MTRenderingSystemNS::getMTTextureDataComponent(iconType);
}

TextureDataComponent * MTRenderingSystem::getTextureDataComponent(WorldEditorIconType iconType)
{
	return MTRenderingSystemNS::getMTTextureDataComponent(iconType);
}

bool MTRenderingSystem::removeMeshDataComponent(EntityID EntityID)
{
	auto l_meshMap = &MTRenderingSystemNS::m_meshMap;
	auto l_mesh = l_meshMap->find(EntityID);
	if (l_mesh != l_meshMap->end())
	{
		g_pCoreSystem->getMemorySystem()->destroyObject(MTRenderingSystemNS::m_MeshDataComponentPool, sizeof(MTMeshDataComponent), l_mesh->second);
		l_meshMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't remove MeshDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

bool MTRenderingSystem::removeTextureDataComponent(EntityID EntityID)
{
	auto l_textureMap = &MTRenderingSystemNS::m_textureMap;
	auto l_texture = l_textureMap->find(EntityID);
	if (l_texture != l_textureMap->end())
	{
		for (auto& i : l_texture->second->m_textureData)
		{
			// @TODO
		}

		g_pCoreSystem->getMemorySystem()->destroyObject(MTRenderingSystemNS::m_TextureDataComponentPool, sizeof(MTTextureDataComponent), l_texture->second);
		l_textureMap->erase(EntityID);
		return true;
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: can't remove TextureDataComponent by EntityID: " + EntityID + " !");
		return false;
	}
}

void MTRenderingSystem::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	MTRenderingSystemNS::m_uninitializedMDC.push(reinterpret_cast<MTMeshDataComponent*>(rhs));
}

void MTRenderingSystem::registerUninitializedTextureDataComponent(TextureDataComponent * rhs)
{
	MTRenderingSystemNS::m_uninitializedTDC.push(reinterpret_cast<MTTextureDataComponent*>(rhs));
}

bool MTRenderingSystem::resize()
{
	return MTRenderingSystemNS::resize();
}

bool MTRenderingSystem::reloadShader(RenderPassType renderPassType)
{
	switch (renderPassType)
	{
	case RenderPassType::Shadow:
		break;
	case RenderPassType::Opaque:
		break;
	case RenderPassType::Light:
		break;
	case RenderPassType::Transparent:
		break;
	case RenderPassType::Terrain:
		break;
	case RenderPassType::PostProcessing:
		break;
	default: break;
	}

	return true;
}

bool MTRenderingSystem::bakeGI()
{
	return true;
}

void MTRenderingSystem::setBridge(MTRenderingSystemBridge* bridge)
{
	MTRenderingSystemNS::m_bridge = bridge;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingSystem: Bridge connected at " + InnoUtility::pointerToString(bridge));
}
