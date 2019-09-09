#include "MTRenderingBackend.h"
#include "../../Component/MTMeshDataComponent.h"
#include "../../Component/MTMaterialDataComponent.h"
#include "../../Component/MTTextureDataComponent.h"

#include "../../ModuleManager/IModuleManager.h"

extern IModuleManager* g_pModuleManager;

INNO_PRIVATE_SCOPE MTRenderingBackendNS
{
	bool setup();
	bool initialize();

	void loadDefaultAssets();
	bool update();
	bool render();
	bool present();
	bool resize();
	bool terminate();

	MTMeshDataComponent* addMTMeshDataComponent();
	MaterialDataComponent* addMTMaterialDataComponent();
	MTTextureDataComponent* addMTTextureDataComponent();

	MTMeshDataComponent* getMTMeshDataComponent(MeshShapeType MeshShapeType);
	MTTextureDataComponent* getMTTextureDataComponent(TextureUsageType TextureUsageType);
	MTTextureDataComponent* getMTTextureDataComponent(FileExplorerIconType iconType);
	MTTextureDataComponent* getMTTextureDataComponent(WorldEditorIconType iconType);

	ObjectStatus m_objectStatus = ObjectStatus::Terminated;
	EntityID m_entityID;

	MTRenderingBackendBridge* m_bridge;

	void* m_MeshDataComponentPool;
	void* m_MaterialDataComponentPool;
	void* m_TextureDataComponentPool;

	ThreadSafeQueue<MTMeshDataComponent*> m_uninitializedMeshes;
	ThreadSafeQueue<MTMaterialDataComponent*> m_uninitializedMaterials;

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

bool MTRenderingBackendNS::setup()
{
	m_entityID = InnoMath::createEntityID();

	bool result = MTRenderingBackendNS::m_bridge->setup();

	m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingBackend setup finished.");
	return result;
}

bool MTRenderingBackendNS::initialize()
{
	m_MeshDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(MTMeshDataComponent), 16384);
	m_MaterialDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(MTMaterialDataComponent), 32768);
	m_TextureDataComponentPool = g_pModuleManager->getMemorySystem()->allocateMemoryPool(sizeof(MTTextureDataComponent), 32768);

	bool result = MTRenderingBackendNS::m_bridge->initialize();

	loadDefaultAssets();

	m_objectStatus = ObjectStatus::Activated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingBackend has been initialized.");
	return result;
}

bool MTRenderingBackendNS::update()
{
	bool result = MTRenderingBackendNS::m_bridge->update();

	return result;
}

bool MTRenderingBackendNS::render()
{
	bool result = MTRenderingBackendNS::m_bridge->render();

	return result;
}

bool MTRenderingBackendNS::present()
{
	bool result = MTRenderingBackendNS::m_bridge->present();

	return result;
}

bool MTRenderingBackendNS::terminate()
{
	bool result = m_bridge->terminate();

	m_objectStatus = ObjectStatus::Terminated;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingBackend has been terminated.");

	return result;
}

void MTRenderingBackendNS::loadDefaultAssets()
{
	auto l_basicNormalTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_normal.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_basicAlbedoTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_albedo.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ALBEDO);
	auto l_basicMetallicTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_metallic.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::METALLIC);
	auto l_basicRoughnessTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_roughness.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::ROUGHNESS);
	auto l_basicAOTDC = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//basic_ao.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::AMBIENT_OCCLUSION);

	auto l_iconTemplate_OBJ = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_OBJ.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PNG = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_PNG.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SHADER = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_SHADER.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_UNKNOWN = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoFileTypeIcons_UNKNOWN.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	auto l_iconTemplate_DirectionalLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_DirectionalLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_PointLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_PointLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);
	auto l_iconTemplate_SphereLight = g_pModuleManager->getAssetSystem()->loadTexture("Res//Textures//InnoWorldEditorIcons_SphereLight.png", TextureSamplerType::SAMPLER_2D, TextureUsageType::NORMAL);

	m_basicNormalTDC = reinterpret_cast<MTTextureDataComponent*>(l_basicNormalTDC);
	m_basicAlbedoTDC = reinterpret_cast<MTTextureDataComponent*>(l_basicAlbedoTDC);
	m_basicMetallicTDC = reinterpret_cast<MTTextureDataComponent*>(l_basicMetallicTDC);
	m_basicRoughnessTDC = reinterpret_cast<MTTextureDataComponent*>(l_basicRoughnessTDC);
	m_basicAOTDC = reinterpret_cast<MTTextureDataComponent*>(l_basicAOTDC);

	m_iconTemplate_OBJ = reinterpret_cast<MTTextureDataComponent*>(l_iconTemplate_OBJ);
	m_iconTemplate_PNG = reinterpret_cast<MTTextureDataComponent*>(l_iconTemplate_PNG);
	m_iconTemplate_SHADER = reinterpret_cast<MTTextureDataComponent*>(l_iconTemplate_SHADER);
	m_iconTemplate_UNKNOWN = reinterpret_cast<MTTextureDataComponent*>(l_iconTemplate_UNKNOWN);

	m_iconTemplate_DirectionalLight = reinterpret_cast<MTTextureDataComponent*>(l_iconTemplate_DirectionalLight);
	m_iconTemplate_PointLight = reinterpret_cast<MTTextureDataComponent*>(l_iconTemplate_PointLight);
	m_iconTemplate_SphereLight = reinterpret_cast<MTTextureDataComponent*>(l_iconTemplate_SphereLight);

	m_unitLineMDC = addMTMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitLine(*m_unitLineMDC);
	m_unitLineMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE_STRIP;
	m_unitLineMDC->m_meshShapeType = MeshShapeType::LINE;
	m_unitLineMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitLineMDC);

	m_unitQuadMDC = addMTMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitQuad(*m_unitQuadMDC);
	m_unitQuadMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitQuadMDC->m_meshShapeType = MeshShapeType::QUAD;
	m_unitQuadMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitQuadMDC);

	m_unitCubeMDC = addMTMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitCube(*m_unitCubeMDC);
	m_unitCubeMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitCubeMDC->m_meshShapeType = MeshShapeType::CUBE;
	m_unitCubeMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitCubeMDC);

	m_unitSphereMDC = addMTMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addUnitSphere(*m_unitSphereMDC);
	m_unitSphereMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_unitSphereMDC->m_meshShapeType = MeshShapeType::SPHERE;
	m_unitSphereMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_unitSphereMDC);

	m_terrainMDC = addMTMeshDataComponent();
	g_pModuleManager->getAssetSystem()->addTerrain(*m_terrainMDC);
	m_terrainMDC->m_meshPrimitiveTopology = MeshPrimitiveTopology::TRIANGLE;
	m_terrainMDC->m_objectStatus = ObjectStatus::Created;
	g_pModuleManager->getPhysicsSystem()->generatePhysicsDataComponent(m_terrainMDC);

	m_bridge->initializeMTMeshDataComponent(m_unitLineMDC);
	m_bridge->initializeMTMeshDataComponent(m_unitQuadMDC);
	m_bridge->initializeMTMeshDataComponent(m_unitCubeMDC);
	m_bridge->initializeMTMeshDataComponent(m_unitSphereMDC);
	m_bridge->initializeMTMeshDataComponent(m_terrainMDC);

	m_bridge->initializeMTTextureDataComponent(m_basicNormalTDC);
	m_bridge->initializeMTTextureDataComponent(m_basicAlbedoTDC);
	m_bridge->initializeMTTextureDataComponent(m_basicMetallicTDC);
	m_bridge->initializeMTTextureDataComponent(m_basicRoughnessTDC);
	m_bridge->initializeMTTextureDataComponent(m_basicAOTDC);

	m_bridge->initializeMTTextureDataComponent(m_iconTemplate_OBJ);
	m_bridge->initializeMTTextureDataComponent(m_iconTemplate_PNG);
	m_bridge->initializeMTTextureDataComponent(m_iconTemplate_SHADER);
	m_bridge->initializeMTTextureDataComponent(m_iconTemplate_UNKNOWN);

	m_bridge->initializeMTTextureDataComponent(m_iconTemplate_DirectionalLight);
	m_bridge->initializeMTTextureDataComponent(m_iconTemplate_PointLight);
	m_bridge->initializeMTTextureDataComponent(m_iconTemplate_SphereLight);
}

MTMeshDataComponent* MTRenderingBackendNS::addMTMeshDataComponent()
{
	static std::atomic<uint32_t> meshCount = 0;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MeshDataComponentPool, sizeof(MTMeshDataComponent));
	auto l_MDC = new(l_rawPtr)MTMeshDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Mesh_" + std::to_string(meshCount) + "/").c_str());
	l_MDC->m_parentEntity = l_parentEntity;
	meshCount++;
	return l_MDC;
}

MaterialDataComponent* MTRenderingBackendNS::addMTMaterialDataComponent()
{
	static std::atomic<uint32_t> materialCount = 0;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_MaterialDataComponentPool, sizeof(MTMaterialDataComponent));
	auto l_MDC = new(l_rawPtr)MTMaterialDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Material_" + std::to_string(materialCount) + "/").c_str());
	l_MDC->m_parentEntity = l_parentEntity;
	materialCount++;
	return l_MDC;
}

MTTextureDataComponent* MTRenderingBackendNS::addMTTextureDataComponent()
{
	static std::atomic<uint32_t> textureCount = 0;
	auto l_rawPtr = g_pModuleManager->getMemorySystem()->spawnObject(m_TextureDataComponentPool, sizeof(MTTextureDataComponent));
	auto l_TDC = new(l_rawPtr)MTTextureDataComponent();
	auto l_parentEntity = g_pModuleManager->getEntityManager()->Spawn(ObjectSource::Runtime, ObjectUsage::Engine, ("Texture_" + std::to_string(textureCount) + "/").c_str());
	l_TDC->m_parentEntity = l_parentEntity;
	textureCount++;
	return l_TDC;
}

MTMeshDataComponent* MTRenderingBackendNS::getMTMeshDataComponent(MeshShapeType meshShapeType)
{
	switch (meshShapeType)
	{
	case MeshShapeType::LINE:
		return MTRenderingBackendNS::m_unitLineMDC; break;
	case MeshShapeType::QUAD:
		return MTRenderingBackendNS::m_unitQuadMDC; break;
	case MeshShapeType::CUBE:
		return MTRenderingBackendNS::m_unitCubeMDC; break;
	case MeshShapeType::SPHERE:
		return MTRenderingBackendNS::m_unitSphereMDC; break;
	case MeshShapeType::TERRAIN:
		return MTRenderingBackendNS::m_terrainMDC; break;
	case MeshShapeType::CUSTOM:
		g_pModuleManager->getLogSystem()->printLog(LogType::INNO_ERROR, "RenderingBackendSystem: wrong MeshShapeType passed to MTRenderingBackend::getMeshDataComponent() !");
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

MTTextureDataComponent * MTRenderingBackendNS::getMTTextureDataComponent(TextureUsageType textureUsageType)
{
	switch (textureUsageType)
	{
	case TextureUsageType::INVISIBLE:
		return nullptr; break;
	case TextureUsageType::NORMAL:
		return MTRenderingBackendNS::m_basicNormalTDC; break;
	case TextureUsageType::ALBEDO:
		return MTRenderingBackendNS::m_basicAlbedoTDC; break;
	case TextureUsageType::METALLIC:
		return MTRenderingBackendNS::m_basicMetallicTDC; break;
	case TextureUsageType::ROUGHNESS:
		return MTRenderingBackendNS::m_basicRoughnessTDC; break;
	case TextureUsageType::AMBIENT_OCCLUSION:
		return MTRenderingBackendNS::m_basicAOTDC; break;
	case TextureUsageType::COLOR_ATTACHMENT:
		return nullptr; break;
	default:
		return nullptr; break;
	}
}

MTTextureDataComponent * MTRenderingBackendNS::getMTTextureDataComponent(FileExplorerIconType iconType)
{
	switch (iconType)
	{
	case FileExplorerIconType::OBJ:
		return MTRenderingBackendNS::m_iconTemplate_OBJ; break;
	case FileExplorerIconType::PNG:
		return MTRenderingBackendNS::m_iconTemplate_PNG; break;
	case FileExplorerIconType::SHADER:
		return MTRenderingBackendNS::m_iconTemplate_SHADER; break;
	case FileExplorerIconType::UNKNOWN:
		return MTRenderingBackendNS::m_iconTemplate_UNKNOWN; break;
	default:
		return nullptr; break;
	}
}

MTTextureDataComponent * MTRenderingBackendNS::getMTTextureDataComponent(WorldEditorIconType iconType)
{
	switch (iconType)
	{
	case WorldEditorIconType::DIRECTIONAL_LIGHT:
		return MTRenderingBackendNS::m_iconTemplate_DirectionalLight; break;
	case WorldEditorIconType::POINT_LIGHT:
		return MTRenderingBackendNS::m_iconTemplate_PointLight; break;
	case WorldEditorIconType::SPHERE_LIGHT:
		return MTRenderingBackendNS::m_iconTemplate_SphereLight; break;
	default:
		return nullptr; break;
	}
}

bool MTRenderingBackendNS::resize()
{
	return true;
}

bool MTRenderingBackend::setup()
{
	return MTRenderingBackendNS::setup();
}

bool MTRenderingBackend::initialize()
{
	return MTRenderingBackendNS::initialize();
}

bool MTRenderingBackend::update()
{
	return MTRenderingBackendNS::update();
}

bool MTRenderingBackend::render()
{
	return MTRenderingBackendNS::render();
}

bool MTRenderingBackend::present()
{
	return MTRenderingBackendNS::present();
}

bool MTRenderingBackend::terminate()
{
	return MTRenderingBackendNS::terminate();
}

ObjectStatus MTRenderingBackend::getStatus()
{
	return MTRenderingBackendNS::m_objectStatus;
}

MeshDataComponent * MTRenderingBackend::addMeshDataComponent()
{
	return MTRenderingBackendNS::addMTMeshDataComponent();
}

MaterialDataComponent * MTRenderingBackend::addMaterialDataComponent()
{
	return MTRenderingBackendNS::addMTMaterialDataComponent();
}

TextureDataComponent * MTRenderingBackend::addTextureDataComponent()
{
	return MTRenderingBackendNS::addMTTextureDataComponent();
}

MeshDataComponent * MTRenderingBackend::getMeshDataComponent(MeshShapeType MeshShapeType)
{
	return MTRenderingBackendNS::getMTMeshDataComponent(MeshShapeType);
}

TextureDataComponent * MTRenderingBackend::getTextureDataComponent(TextureUsageType TextureUsageType)
{
	return MTRenderingBackendNS::getMTTextureDataComponent(TextureUsageType);
}

TextureDataComponent * MTRenderingBackend::getTextureDataComponent(FileExplorerIconType iconType)
{
	return MTRenderingBackendNS::getMTTextureDataComponent(iconType);
}

TextureDataComponent * MTRenderingBackend::getTextureDataComponent(WorldEditorIconType iconType)
{
	return MTRenderingBackendNS::getMTTextureDataComponent(iconType);
}

void MTRenderingBackend::registerUninitializedMeshDataComponent(MeshDataComponent * rhs)
{
	MTRenderingBackendNS::m_uninitializedMeshes.push(reinterpret_cast<MTMeshDataComponent*>(rhs));
}

void MTRenderingBackend::registerUninitializedMaterialDataComponent(MaterialDataComponent* rhs)
{
	MTRenderingBackendNS::m_uninitializedMaterials.push(reinterpret_cast<MTMaterialDataComponent*>(rhs));
}

bool MTRenderingBackend::resize()
{
	return MTRenderingBackendNS::resize();
}

bool MTRenderingBackend::reloadShader(RenderPassType renderPassType)
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

bool MTRenderingBackend::bakeGI()
{
	return true;
}

void MTRenderingBackend::setBridge(MTRenderingBackendBridge* bridge)
{
	MTRenderingBackendNS::m_bridge = bridge;
	g_pModuleManager->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MTRenderingBackend: Bridge connected at " + InnoUtility::pointerToString(bridge));
}
