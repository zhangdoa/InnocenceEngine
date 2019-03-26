#include "DXRenderingSystem.h"

#include "../../component/DX11RenderingSystemComponent.h"
#include "../../component/WinWindowSystemComponent.h"

#include "DXRenderingSystemUtilities.h"

#pragma comment(lib, "d3d12.lib")
#pragma comment(lib, "dxgi.lib")
#pragma comment(lib, "d3dcompiler.lib")

#include "../ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE DXRenderingSystemNS
{
	ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;

	bool setup(IRenderingFrontendSystem* renderingFrontend);
	bool update();
	bool terminate();

	bool initializeDefaultAssets();

	void prepareRenderingData();

	static DX11RenderingSystemComponent* g_DXRenderingSystemComponent;

	bool createPhysicalDevices();
	bool createSwapChain();
	bool createBackBuffer();
	bool createRasterizer();

	IRenderingFrontendSystem* m_renderingFrontendSystem;

	std::vector<MeshDataPack> m_meshDataPack;
}

bool DXRenderingSystemNS::createPhysicalDevices()
{
	return true;
}

bool DXRenderingSystemNS::createSwapChain()
{
	return true;
}

bool DXRenderingSystemNS::createBackBuffer()
{
	return true;
}

bool DXRenderingSystemNS::createRasterizer()
{
	return true;
}

bool DXRenderingSystemNS::setup(IRenderingFrontendSystem* renderingFrontend)
{
	m_renderingFrontendSystem = renderingFrontend;

	g_DXRenderingSystemComponent = &DX11RenderingSystemComponent::get();

	bool result = true;
	result = result && initializeComponentPool();
	result = result && createPhysicalDevices();
	result = result && createSwapChain();
	result = result && createBackBuffer();
	result = result && createRasterizer();

	auto l_screenResolution = m_renderingFrontendSystem->getScreenResolution();

	// Setup the description of the deferred pass.
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureSamplerType = TextureSamplerType::SAMPLER_2D;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureUsageType = TextureUsageType::RENDER_TARGET;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureColorComponentsFormat = TextureColorComponentsFormat::RGBA16F;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.texturePixelDataFormat = TexturePixelDataFormat::RGBA;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureMinFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureMagFilterMethod = TextureFilterMethod::NEAREST;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureWrapMethod = TextureWrapMethod::CLAMP_TO_EDGE;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureWidth = l_screenResolution.x;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.textureHeight = l_screenResolution.y;
	g_DXRenderingSystemComponent->deferredPassTextureDesc.texturePixelDataType = TexturePixelDataType::FLOAT;

	g_DXRenderingSystemComponent->deferredPassRTVDesc.Format = DXGI_FORMAT_R16G16B16A16_FLOAT;
	g_DXRenderingSystemComponent->deferredPassRTVDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	g_DXRenderingSystemComponent->deferredPassRTVDesc.Texture2D.MipSlice = 0;

	m_objectStatus = ObjectStatus::ALIVE;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem setup finished.");
	return result;
}

bool DXRenderingSystemNS::update()
{
	if (m_renderingFrontendSystem->anyUninitializedMeshDataComponent())
	{
		auto l_MDC = m_renderingFrontendSystem->acquireUninitializedMeshDataComponent();
		if (l_MDC)
		{
			auto l_result = generateDX12MeshDataComponent(l_MDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create DXMeshDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}
	if (m_renderingFrontendSystem->anyUninitializedTextureDataComponent())
	{
		auto l_TDC = m_renderingFrontendSystem->acquireUninitializedTextureDataComponent();
		if (l_TDC)
		{
			auto l_result = generateDX12TextureDataComponent(l_TDC);
			if (l_result == nullptr)
			{
				g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, "DXRenderingSystem: can't create DXTextureDataComponent for " + l_result->m_parentEntity + "!");
			}
		}
	}

	// Clear the buffers to begin the scene.
	prepareRenderingData();

	DXGeometryRenderingPassUtilities::update();

	DXLightRenderingPassUtilities::update();

	DXFinalRenderingPassUtilities::update();

	return true;
}

bool DXRenderingSystemNS::terminate()
{
	m_objectStatus = ObjectStatus::SHUTDOWN;
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem has been terminated.");
	return true;
}

bool  DXRenderingSystemNS::initializeDefaultAssets()
{
	auto l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::LINE);
	g_DXRenderingSystemComponent->m_UnitLineDXMDC = generateDX11MeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::QUAD);
	g_DXRenderingSystemComponent->m_UnitQuadDXMDC = generateDX11MeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::CUBE);
	g_DXRenderingSystemComponent->m_UnitCubeDXMDC = generateDX11MeshDataComponent(l_MDC);

	l_MDC = g_pCoreSystem->getAssetSystem()->getMeshDataComponent(MeshShapeType::SPHERE);
	g_DXRenderingSystemComponent->m_UnitSphereDXMDC = generateDX11MeshDataComponent(l_MDC);

	g_DXRenderingSystemComponent->m_basicNormalDXTDC = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::NORMAL));
	g_DXRenderingSystemComponent->m_basicAlbedoDXTDC = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ALBEDO));
	g_DXRenderingSystemComponent->m_basicMetallicDXTDC = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::METALLIC));
	g_DXRenderingSystemComponent->m_basicRoughnessDXTDC = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::ROUGHNESS));
	g_DXRenderingSystemComponent->m_basicAODXTDC = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(TextureUsageType::AMBIENT_OCCLUSION));

	g_DXRenderingSystemComponent->m_iconTemplate_OBJ = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::OBJ));
	g_DXRenderingSystemComponent->m_iconTemplate_PNG = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::PNG));
	g_DXRenderingSystemComponent->m_iconTemplate_SHADER = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::SHADER));
	g_DXRenderingSystemComponent->m_iconTemplate_UNKNOWN = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(FileExplorerIconType::UNKNOWN));

	g_DXRenderingSystemComponent->m_iconTemplate_DirectionalLight = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::DIRECTIONAL_LIGHT));
	g_DXRenderingSystemComponent->m_iconTemplate_PointLight = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::POINT_LIGHT));
	g_DXRenderingSystemComponent->m_iconTemplate_SphereLight = generateDX11TextureDataComponent(g_pCoreSystem->getAssetSystem()->getTextureDataComponent(WorldEditorIconType::SPHERE_LIGHT));

	return true;
}

void DXRenderingSystemNS::prepareRenderingData()
{
	auto l_cameraDataPack = m_renderingFrontendSystem->getCameraDataPack();

	g_DXRenderingSystemComponent->m_GPassCameraCBufferData.m_CamProjJittered = l_cameraDataPack.p_Jittered;
	g_DXRenderingSystemComponent->m_GPassCameraCBufferData.m_CamProjOriginal = l_cameraDataPack.p_Original;
	g_DXRenderingSystemComponent->m_GPassCameraCBufferData.m_CamRot = l_cameraDataPack.r;
	g_DXRenderingSystemComponent->m_GPassCameraCBufferData.m_CamTrans = l_cameraDataPack.t;
	g_DXRenderingSystemComponent->m_GPassCameraCBufferData.m_CamRot_prev = l_cameraDataPack.r_prev;
	g_DXRenderingSystemComponent->m_GPassCameraCBufferData.m_CamTrans_prev = l_cameraDataPack.t_prev;

	auto l_sunDataPack = m_renderingFrontendSystem->getSunDataPack();

	g_DXRenderingSystemComponent->m_LPassCBufferData.viewPos = l_cameraDataPack.globalPos;
	g_DXRenderingSystemComponent->m_LPassCBufferData.lightDir = l_sunDataPack.dir;
	g_DXRenderingSystemComponent->m_LPassCBufferData.color = l_sunDataPack.luminance;

	auto l_meshDataPack = m_renderingFrontendSystem->getMeshDataPack();

	if (l_meshDataPack.has_value())
	{
		m_meshDataPack = l_meshDataPack.value();
	}

	for (auto i : m_meshDataPack)
	{
		auto l_DXMDC = getDX11MeshDataComponent(i.MDC->m_parentEntity);
		if (l_DXMDC && l_DXMDC->m_objectStatus == ObjectStatus::ALIVE)
		{
			GPassMeshDataPack l_meshDataPack;

			l_meshDataPack.indiceSize = i.MDC->m_indicesSize;
			l_meshDataPack.meshPrimitiveTopology = i.MDC->m_meshPrimitiveTopology;
			l_meshDataPack.meshCBuffer.m = i.m;
			l_meshDataPack.meshCBuffer.m_prev = i.m_prev;
			l_meshDataPack.meshCBuffer.m_normalMat = i.normalMat;
			l_meshDataPack.DXMDC = l_DXMDC;

			auto l_material = i.material;
			// any normal?
			auto l_TDC = l_material->m_texturePack.m_normalTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.normalDXTDC = getDX11TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useNormalTexture = false;
			}
			// any albedo?
			l_TDC = l_material->m_texturePack.m_albedoTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.albedoDXTDC = getDX11TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useAlbedoTexture = false;
			}
			// any metallic?
			l_TDC = l_material->m_texturePack.m_metallicTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.metallicDXTDC = getDX11TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useMetallicTexture = false;
			}
			// any roughness?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.roughnessDXTDC = getDX11TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useRoughnessTexture = false;
			}
			// any ao?
			l_TDC = l_material->m_texturePack.m_roughnessTDC.second;
			if (l_TDC && l_TDC->m_objectStatus == ObjectStatus::ALIVE)
			{
				l_meshDataPack.AODXTDC = getDX11TextureDataComponent(l_TDC->m_parentEntity);
			}
			else
			{
				l_meshDataPack.textureCBuffer.useAOTexture = false;
			}

			l_meshDataPack.textureCBuffer.albedo = vec4(
				l_material->m_meshCustomMaterial.albedo_r,
				l_material->m_meshCustomMaterial.albedo_g,
				l_material->m_meshCustomMaterial.albedo_b,
				1.0f
			);
			l_meshDataPack.textureCBuffer.MRA = vec4(
				l_material->m_meshCustomMaterial.metallic,
				l_material->m_meshCustomMaterial.roughness,
				l_material->m_meshCustomMaterial.ao,
				1.0f
			);

			g_DXRenderingSystemComponent->m_GPassMeshDataQueue.push(l_meshDataPack);
		}
	}
}

bool DXRenderingSystem::setup(IRenderingFrontendSystem* renderingFrontend)
{
	return DXRenderingSystemNS::setup(renderingFrontend);
}

bool DXRenderingSystem::initialize()
{
	DXRenderingSystemNS::initializeDefaultAssets();
	DXGeometryRenderingPassUtilities::initialize();
	DXLightRenderingPassUtilities::initialize();
	DXFinalRenderingPassUtilities::initialize();

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "DXRenderingSystem has been initialized.");
	return true;
}

bool DXRenderingSystem::update()
{
	return DXRenderingSystemNS::update();
}

bool DXRenderingSystem::terminate()
{
	return DXRenderingSystemNS::terminate();
}

ObjectStatus DXRenderingSystem::getStatus()
{
	return DXRenderingSystemNS::m_objectStatus;
}

bool DXRenderingSystem::resize()
{
	return true;
}

bool DXRenderingSystem::reloadShader(RenderPassType renderPassType)
{
	return true;
}

bool DXRenderingSystem::bakeGI()
{
	return true;
}