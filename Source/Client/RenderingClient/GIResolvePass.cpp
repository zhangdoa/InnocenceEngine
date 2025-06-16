#include "GIResolvePass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"
#include "../../Engine/Common/TaskScheduler.h"
#include "../../Engine/Common/IOService.h"
#include "../../Engine/Services/HIDService.h"
#include "../../Engine/Services/SceneService.h"
#include "../../Engine/Common/Array.h"
#include "../../Engine/Services/ComponentManager.h"

#include "GIDataLoader.h"
#include "SunShadowBlurEvenPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;




namespace GIResolvePass
{
	bool setupSky();
	bool setupSurfels();
	bool setupBricks();
	bool setupProbes();
	bool setupIrradianceVolume();

	bool generateSkyRadiance();
	bool generateSkyIrradiance();
	bool litSurfels();
	bool litBricks();
	bool litProbes();
	bool generateIrradianceVolume();

	RenderPassComponent* m_skyRadianceRenderPassComp;
	ShaderProgramComponent* m_skyRadianceSPC;

	RenderPassComponent* m_skyIrradianceRenderPassComp;
	ShaderProgramComponent* m_skyIrradianceSPC;

	RenderPassComponent* m_surfelRenderPassComp = 0;
	ShaderProgramComponent* m_surfelSPC = 0;

	RenderPassComponent* m_brickRenderPassComp = 0;
	ShaderProgramComponent* m_brickSPC = 0;

	RenderPassComponent* m_probeRenderPassComp = 0;
	ShaderProgramComponent* m_probeSPC = 0;

	RenderPassComponent* m_irradianceVolumeRenderPassComp = 0;
	ShaderProgramComponent* m_irradianceVolumeSPC = 0;
	SamplerComponent* m_irradianceVolumeSamplerComp = 0;

	TextureComponent* m_skyRadianceVolume = 0;
	GPUBufferComponent* m_skyIrradianceGPUBufferComp = 0;

	GPUBufferComponent* m_surfelGPUBufferComp = 0;
	GPUBufferComponent* m_surfelIrradianceGPUBufferComp = 0;
	GPUBufferComponent* m_brickGPUBufferComp = 0;
	GPUBufferComponent* m_brickIrradianceGPUBufferComp = 0;
	GPUBufferComponent* m_brickFactorGPUBufferComp = 0;
	GPUBufferComponent* m_probeGPUBufferComp = 0;
	TextureComponent* m_probeVolume = 0;
	TextureComponent* m_irradianceVolume = 0;

	Vec4 m_minProbePos;
	Vec4 m_irradianceVolumePosOffset;
	Vec4 m_irradianceVolumeRange;

	std::function<void()> f_sceneLoadingFinishedCallback;
	std::function<void()> f_sceneLoadingStartedCallback;
	std::function<void()> f_reloadGIData;
	bool m_needToReloadGIData = false;

	bool m_GIDataLoaded = false;
}

bool GIResolvePass::InitializeGPUBuffers()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_surfels = GIDataLoader::GetSurfels();

	if (l_surfels.size())
	{
		ITask::Desc taskDesc("GIResolvePassInitializeGPUBuffersTask", ITask::Type::Once, 2);
		auto l_GIResolvePassInitializeGPUBuffersTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc,
			[&]() {
				m_surfelGPUBufferComp = l_renderingServer->AddGPUBufferComponent("SurfelGPUBuffer/");
				m_surfelGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_surfelGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_surfelGPUBufferComp->m_ElementCount = l_surfels.size();
				m_surfelGPUBufferComp->m_ElementSize = sizeof(Surfel);
				m_surfelGPUBufferComp->m_InitialData = &l_surfels[0];

				l_renderingServer->Initialize(m_surfelGPUBufferComp);

				m_surfelIrradianceGPUBufferComp = l_renderingServer->AddGPUBufferComponent("SurfelIrradianceGPUBuffer/");
				m_surfelIrradianceGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_surfelIrradianceGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_surfelIrradianceGPUBufferComp->m_ElementCount = l_surfels.size();
				m_surfelIrradianceGPUBufferComp->m_ElementSize = sizeof(Vec4);

				l_renderingServer->Initialize(m_surfelIrradianceGPUBufferComp);

				auto l_bricks = GIDataLoader::GetBricks();

				auto l_bricksCount = l_bricks.size();

				auto l_min = Math::maxVec4<float>;
				auto l_max = Math::minVec4<float>;

				for (size_t i = 0; i < l_bricksCount; i++)
				{
					l_min = Math::elementWiseMin(l_min, l_bricks[i].boundBox.m_boundMin);
					l_max = Math::elementWiseMax(l_max, l_bricks[i].boundBox.m_boundMax);
				}
				m_irradianceVolumeRange = l_max - l_min;
				m_irradianceVolumePosOffset = l_min;

				std::vector<uint32_t> l_brickConstantBuffer;

				l_brickConstantBuffer.resize(l_bricks.size() * 2);
				for (size_t i = 0; i < l_bricks.size(); i++)
				{
					l_brickConstantBuffer[2 * i] = l_bricks[i].surfelRangeBegin;
					l_brickConstantBuffer[2 * i + 1] = l_bricks[i].surfelRangeEnd;
				}

				m_brickGPUBufferComp = l_renderingServer->AddGPUBufferComponent("BrickGPUBuffer/");
				m_brickGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_brickGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickGPUBufferComp->m_ElementCount = l_bricks.size();
				m_brickGPUBufferComp->m_ElementSize = sizeof(uint32_t) * 2;
				m_brickGPUBufferComp->m_InitialData = &l_brickConstantBuffer[0];

				l_renderingServer->Initialize(m_brickGPUBufferComp);

				m_brickIrradianceGPUBufferComp = l_renderingServer->AddGPUBufferComponent("BrickIrradianceGPUBuffer/");
				m_brickIrradianceGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_brickIrradianceGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickIrradianceGPUBufferComp->m_ElementCount = l_bricks.size();
				m_brickIrradianceGPUBufferComp->m_ElementSize = sizeof(Vec4);

				l_renderingServer->Initialize(m_brickIrradianceGPUBufferComp);

				auto l_brickFactors = GIDataLoader::GetBrickFactors();

				m_brickFactorGPUBufferComp = l_renderingServer->AddGPUBufferComponent("BrickFactorGPUBuffer/");
				m_brickFactorGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_brickFactorGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickFactorGPUBufferComp->m_ElementCount = l_brickFactors.size();
				m_brickFactorGPUBufferComp->m_ElementSize = sizeof(BrickFactor);
				m_brickFactorGPUBufferComp->m_InitialData = &l_brickFactors[0];

				l_renderingServer->Initialize(m_brickFactorGPUBufferComp);

				std::vector<Probe> l_probes = GIDataLoader::GetProbes();

				TVec4<uint32_t> l_probeIndex;
				float l_minPos;

				std::sort(l_probes.begin(), l_probes.end(), [&](Probe A, Probe B)
					{
						return A.pos.x < B.pos.x;
					});

				l_minPos = l_probes[0].pos.x;
				m_minProbePos.x = l_minPos;

				for (size_t i = 0; i < l_probes.size(); i++)
				{
					auto l_probePosWS = l_probes[i].pos;

					if ((l_probePosWS.x - l_minPos) > epsilon<float, 2>)
					{
						l_minPos = l_probePosWS.x;
						l_probeIndex.x++;
					}

					l_probes[i].pos.x = (float)l_probeIndex.x;
				}

				std::sort(l_probes.begin(), l_probes.end(), [&](Probe A, Probe B)
					{
						return A.pos.y < B.pos.y;
					});

				l_minPos = l_probes[0].pos.y;
				m_minProbePos.y = l_minPos;

				for (size_t i = 0; i < l_probes.size(); i++)
				{
					auto l_probePosWS = l_probes[i].pos;

					if ((l_probePosWS.y - l_minPos) > epsilon<float, 2>)
					{
						l_minPos = l_probePosWS.y;
						l_probeIndex.y++;
					}

					l_probes[i].pos.y = (float)l_probeIndex.y;
				}

				std::sort(l_probes.begin(), l_probes.end(), [&](Probe A, Probe B)
					{
						return A.pos.z < B.pos.z;
					});

				l_minPos = l_probes[0].pos.z;
				m_minProbePos.z = l_minPos;

				for (size_t i = 0; i < l_probes.size(); i++)
				{
					auto l_probePosWS = l_probes[i].pos;

					if ((l_probePosWS.z - l_minPos) > epsilon<float, 2>)
					{
						l_minPos = l_probePosWS.z;
						l_probeIndex.z++;
					}

					l_probes[i].pos.z = (float)l_probeIndex.z;
				}

				m_minProbePos.w = 1.0f;

				m_probeGPUBufferComp = l_renderingServer->AddGPUBufferComponent("ProbeGPUBuffer/");
				m_probeGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_probeGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_probeGPUBufferComp->m_ElementCount = l_probes.size();
				m_probeGPUBufferComp->m_ElementSize = sizeof(Probe);
				m_probeGPUBufferComp->m_InitialData = &l_probes[0];

				l_renderingServer->Initialize(m_probeGPUBufferComp);

				auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

				m_probeVolume = l_renderingServer->AddTextureComponent("ProbeVolume/");
				m_probeVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

				m_probeVolume->m_TextureDesc.Width = (uint32_t)l_probeIndex.x + 1;
				m_probeVolume->m_TextureDesc.Height = (uint32_t)l_probeIndex.y + 1;
				m_probeVolume->m_TextureDesc.DepthOrArraySize = ((uint32_t)l_probeIndex.z + 1) * 6;
				m_probeVolume->m_TextureDesc.Usage = TextureUsage::Sample;
				m_probeVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
				m_probeVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

				l_renderingServer->Initialize(m_probeVolume);

				m_irradianceVolume = l_renderingServer->AddTextureComponent("IrradianceVolume/");
				m_irradianceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

				m_irradianceVolume->m_TextureDesc.Width = 64;
				m_irradianceVolume->m_TextureDesc.Height = 32;
				m_irradianceVolume->m_TextureDesc.DepthOrArraySize = 64 * 6;
				m_irradianceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
				m_irradianceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
				m_irradianceVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

				l_renderingServer->Initialize(m_irradianceVolume);

				m_GIDataLoaded = true;
			});

		l_GIResolvePassInitializeGPUBuffersTask->Activate();
		l_GIResolvePassInitializeGPUBuffersTask->Wait();
	}

	return true;
}

bool GIResolvePass::DeleteGPUBuffers()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	ITask::Desc taskDesc("GIResolvePassDeleteGPUBuffersTask", ITask::Type::Once, 2);
	auto l_GIResolvePassDeleteGPUBuffersTask = g_Engine->Get<TaskScheduler>()->Submit(taskDesc,
		[&]() {
			if (m_surfelGPUBufferComp)
			{
				l_renderingServer->Delete(m_surfelGPUBufferComp);
			}
			if (m_surfelIrradianceGPUBufferComp)
			{
				l_renderingServer->Delete(m_surfelIrradianceGPUBufferComp);
			}
			if (m_brickGPUBufferComp)
			{
				l_renderingServer->Delete(m_brickGPUBufferComp);
			}
			if (m_brickIrradianceGPUBufferComp)
			{
				l_renderingServer->Delete(m_brickIrradianceGPUBufferComp);
			}
			if (m_brickFactorGPUBufferComp)
			{
				l_renderingServer->Delete(m_brickFactorGPUBufferComp);
			}
			if (m_probeGPUBufferComp)
			{
				l_renderingServer->Delete(m_probeGPUBufferComp);
			}
			if (m_probeVolume)
			{
				l_renderingServer->Delete(m_probeVolume);
			}

			m_GIDataLoaded = false;
		});

	l_GIResolvePassDeleteGPUBuffersTask->Activate();
	l_GIResolvePassDeleteGPUBuffersTask->Wait();

	return true;
}

bool GIResolvePass::Setup()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	f_reloadGIData = [&]() { m_needToReloadGIData = true; };
	g_Engine->Get<HIDService>()->AddButtonStateCallback(ButtonState{ INNO_KEY_B, true }, ButtonEvent{ EventLifeTime::OneShot, &f_reloadGIData });

	setupSky();
	setupSurfels();
	setupBricks();
	setupProbes();
	setupIrradianceVolume();

	f_sceneLoadingStartedCallback = []() { DeleteGPUBuffers(); };
	f_sceneLoadingFinishedCallback = []() { InitializeGPUBuffers(); };

	g_Engine->Get<SceneService>()->AddSceneLoadingStartedCallback(&f_sceneLoadingStartedCallback, 0);
	g_Engine->Get<SceneService>()->AddSceneLoadingFinishedCallback(&f_sceneLoadingFinishedCallback, 0);

	return true;
}

bool GIResolvePass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Initialize(m_skyRadianceSPC);
	l_renderingServer->Initialize(m_skyRadianceRenderPassComp);

	l_renderingServer->Initialize(m_skyIrradianceSPC);
	l_renderingServer->Initialize(m_skyIrradianceRenderPassComp);

	l_renderingServer->Initialize(m_skyRadianceVolume);
	l_renderingServer->Initialize(m_skyIrradianceGPUBufferComp);

	l_renderingServer->Initialize(m_surfelSPC);
	l_renderingServer->Initialize(m_surfelRenderPassComp);

	l_renderingServer->Initialize(m_brickSPC);
	l_renderingServer->Initialize(m_brickRenderPassComp);

	l_renderingServer->Initialize(m_probeSPC);
	l_renderingServer->Initialize(m_probeRenderPassComp);

	l_renderingServer->Initialize(m_irradianceVolumeSPC);
	l_renderingServer->Initialize(m_irradianceVolumeRenderPassComp);
	l_renderingServer->Initialize(m_irradianceVolumeSamplerComp);

	return true;
}

bool GIResolvePass::setupSky()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_skyRadianceSPC = l_renderingServer->AddShaderProgramComponent("GIResolveSkyRadiancePass/");
	m_skyRadianceSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSkyRadiancePass.comp/";

	m_skyRadianceRenderPassComp = l_renderingServer->AddRenderPassComponent("GIResolveSkyRadiancePass/");

	m_skyRadianceRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs.resize(2);
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_skyRadianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_skyRadianceRenderPassComp->m_ShaderProgram = m_skyRadianceSPC;

	////
	m_skyIrradianceSPC = l_renderingServer->AddShaderProgramComponent("GIResolveSkyIrradiancePass/");
	m_skyIrradianceSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSkyIrradiancePass.comp/";

	m_skyIrradianceRenderPassComp = l_renderingServer->AddRenderPassComponent("GIResolveSkyIrradiancePass/");

	m_skyIrradianceRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs.resize(2);
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceRenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_skyIrradianceRenderPassComp->m_ShaderProgram = m_skyIrradianceSPC;

	////
	m_skyRadianceVolume = l_renderingServer->AddTextureComponent("SkyRadianceVolume/");
	m_skyRadianceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_skyRadianceVolume->m_TextureDesc.Width = 8;
	m_skyRadianceVolume->m_TextureDesc.Height = 8;
	m_skyRadianceVolume->m_TextureDesc.DepthOrArraySize = 6;
	m_skyRadianceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_skyRadianceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_skyRadianceVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	m_skyIrradianceGPUBufferComp = l_renderingServer->AddGPUBufferComponent("SkyIrradianceGPUBuffer/");
	m_skyIrradianceGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceGPUBufferComp->m_ElementCount = 6;
	m_skyIrradianceGPUBufferComp->m_ElementSize = sizeof(Vec4);

	return true;
}

bool GIResolvePass::setupSurfels()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_surfelSPC = l_renderingServer->AddShaderProgramComponent("GIResolveSurfelPass/");
	m_surfelSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSurfelPass.comp/";

	m_surfelRenderPassComp = l_renderingServer->AddRenderPassComponent("GIResolveSurfelPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_surfelRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs.resize(7);
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 5;

	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 6;

	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 8;

	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;

	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;

	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_surfelRenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_surfelRenderPassComp->m_ShaderProgram = m_surfelSPC;

	return true;
}

bool GIResolvePass::setupBricks()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_brickSPC = l_renderingServer->AddShaderProgramComponent("GIResolveBrickPass/");
	m_brickSPC->m_ShaderFilePaths.m_CSPath = "GIResolveBrickPass.comp/";

	m_brickRenderPassComp = l_renderingServer->AddRenderPassComponent("GIResolveBrickPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_brickRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_brickRenderPassComp->m_ResourceBindingLayoutDescs.resize(5);
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 6;

	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 8;

	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;

	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_brickRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;

	m_brickRenderPassComp->m_ShaderProgram = m_brickSPC;

	return true;
}

bool GIResolvePass::setupProbes()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_probeSPC = l_renderingServer->AddShaderProgramComponent("GIResolveProbePass/");
	m_probeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveProbePass.comp/";

	m_probeRenderPassComp = l_renderingServer->AddRenderPassComponent("GIResolveProbePass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_probeRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_probeRenderPassComp->m_ResourceBindingLayoutDescs.resize(7);
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 6;

	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 8;

	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;

	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;

	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;

	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_probeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_probeRenderPassComp->m_ShaderProgram = m_probeSPC;

	return true;
}

bool GIResolvePass::setupIrradianceVolume()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_irradianceVolumeSPC = l_renderingServer->AddShaderProgramComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveIrradianceVolumePass.comp/";

	m_irradianceVolumeRenderPassComp = l_renderingServer->AddRenderPassComponent("GIResolveIrradianceVolumePass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_irradianceVolumeRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs.resize(7);
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 6;

	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 8;

	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;

	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Sampler;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 3;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_irradianceVolumeRenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_irradianceVolumeRenderPassComp->m_ShaderProgram = m_irradianceVolumeSPC;
	m_irradianceVolumeSamplerComp = l_renderingServer->AddSamplerComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Border;
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Border;
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_WrapMethodW = TextureWrapMethod::Border;
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_BorderColor[3] = 1.0f;

	return true;
}

bool GIResolvePass::generateSkyRadiance()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	// l_renderingServer->CommandListBegin(m_skyRadianceRenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_skyRadianceRenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_skyRadianceRenderPassComp);

	// l_renderingServer->BindGPUResource(m_skyRadianceRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_skyRadianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 1);

	// l_renderingServer->Dispatch(m_skyRadianceRenderPassComp, 1, 1, 1);

	// l_renderingServer->UnbindGPUResource(m_skyRadianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 1);

	// l_renderingServer->CommandListEnd(m_skyRadianceRenderPassComp);

	return true;
}

bool GIResolvePass::generateSkyIrradiance()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	// l_renderingServer->CommandListBegin(m_skyIrradianceRenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_skyIrradianceRenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_skyIrradianceRenderPassComp);

	// l_renderingServer->BindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 0);
	// l_renderingServer->BindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 1);

	// l_renderingServer->Dispatch(m_skyIrradianceRenderPassComp, 1, 1, 1);

	// l_renderingServer->UnbindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 0);
	// l_renderingServer->UnbindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 1);

	// l_renderingServer->CommandListEnd(m_skyIrradianceRenderPassComp);

	return true;
}

bool GIResolvePass::litSurfels()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	// TODO: Implement per-pass dispatch params buffer for GIResolvePass
	// auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_CSMGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::CSM);
	auto l_GIGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = std::ceil((double)m_surfelGPUBufferComp->m_ElementCount / (double)(l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide));
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_surfelLitWorkload;
	l_surfelLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_surfelLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// l_renderingServer->Upload(l_dispatchParamsGPUBufferComp, &l_surfelLitWorkload, 2, 1);

	// l_renderingServer->CommandListBegin(m_surfelRenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_surfelRenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_surfelRenderPassComp);

	// l_renderingServer->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_CSMGPUBufferComp, 1);
	// l_renderingServer->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 2);
	// l_renderingServer->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 3);
	// l_renderingServer->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelGPUBufferComp, 4);
	// l_renderingServer->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 5);
	// l_renderingServer->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6);

	// l_renderingServer->Dispatch(m_surfelRenderPassComp, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	// l_renderingServer->UnbindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelGPUBufferComp, 4);
	// l_renderingServer->UnbindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 5);
	// l_renderingServer->UnbindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6);

	// l_renderingServer->CommandListEnd(m_surfelRenderPassComp);

	return true;
}

bool GIResolvePass::litBricks()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	// TODO: Implement per-pass dispatch params buffer for GIResolvePass
	// auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_brickGPUBufferComp->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_brickLitWorkload;
	l_brickLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_brickLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// l_renderingServer->Upload(l_dispatchParamsGPUBufferComp, &l_brickLitWorkload, 3, 1);

	// l_renderingServer->CommandListBegin(m_brickRenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_brickRenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_brickRenderPassComp);

	// l_renderingServer->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 1);
	// l_renderingServer->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickGPUBufferComp, 2);
	// l_renderingServer->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 3);
	// l_renderingServer->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 4);

	// l_renderingServer->Dispatch(m_brickRenderPassComp, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	// l_renderingServer->UnbindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickGPUBufferComp, 2);
	// l_renderingServer->UnbindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 3);
	// l_renderingServer->UnbindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 4);

	// l_renderingServer->CommandListEnd(m_brickRenderPassComp);

	return true;
}

bool GIResolvePass::litProbes()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	// TODO: Implement per-pass dispatch params buffer for GIResolvePass
	// auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_probeGPUBufferComp->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_probeLitWorkload;
	l_probeLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_probeLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// l_renderingServer->Upload(l_dispatchParamsGPUBufferComp, &l_probeLitWorkload, 4, 1);

	// l_renderingServer->CommandListBegin(m_probeRenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_probeRenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_probeRenderPassComp);

	// l_renderingServer->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1);
	// l_renderingServer->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 2);
	// l_renderingServer->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeGPUBufferComp, 3);
	// l_renderingServer->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickFactorGPUBufferComp, 4);
	// l_renderingServer->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 5);
	// l_renderingServer->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeVolume, 6);

	// l_renderingServer->Dispatch(m_probeRenderPassComp, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	// l_renderingServer->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeGPUBufferComp, 3);
	// l_renderingServer->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickFactorGPUBufferComp, 4);
	// l_renderingServer->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 5);
	// l_renderingServer->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeVolume, 6);

	// l_renderingServer->CommandListEnd(m_probeRenderPassComp);

	return true;
}

bool GIResolvePass::generateIrradianceVolume()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	// TODO: Implement per-pass dispatch params buffer for GIResolvePass
	// auto l_dispatchParamsGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_numThreadsX = 64;
	auto l_numThreadsY = 32;
	auto l_numThreadsZ = 64;

	DispatchParamsConstantBuffer l_irradianceVolumeLitWorkload;
	l_irradianceVolumeLitWorkload.numThreadGroups = TVec4<uint32_t>(8, 4, 8, 0);
	l_irradianceVolumeLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// l_renderingServer->Upload(l_dispatchParamsGPUBufferComp, &l_irradianceVolumeLitWorkload, 5, 1);

	// l_renderingServer->CommandListBegin(m_irradianceVolumeRenderPassComp, 0);
	// l_renderingServer->BindRenderPassComponent(m_irradianceVolumeRenderPassComp);
	// l_renderingServer->ClearRenderTargets(m_irradianceVolumeRenderPassComp);

	// l_renderingServer->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_irradianceVolumeSamplerComp, 6);

	// l_renderingServer->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_renderingServer->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1);
	// l_renderingServer->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 2);
	// l_renderingServer->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_probeVolume, 3);
	// l_renderingServer->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_irradianceVolume, 4);
	// l_renderingServer->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 5);

	// l_renderingServer->Dispatch(m_irradianceVolumeRenderPassComp, 8, 4, 8);

	// l_renderingServer->UnbindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_probeVolume, 3);
	// l_renderingServer->UnbindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_irradianceVolume, 4);
	// l_renderingServer->UnbindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 5);

	// l_renderingServer->CommandListEnd(m_irradianceVolumeRenderPassComp);

	return true;
}

bool GIResolvePass::PrepareCommandList()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	if (m_needToReloadGIData)
	{
		GIDataLoader::ReloadGIData();
		m_needToReloadGIData = false;
		DeleteGPUBuffers();
		InitializeGPUBuffers();
	}

	if (m_GIDataLoaded)
	{
		auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
		auto l_GIGPUBufferComp = g_Engine->Get<RenderingContextService>()->GetGPUBufferComponent(GPUBufferUsageType::GI);
		auto l_cameraConstantBuffer = g_Engine->Get<RenderingContextService>()->GetPerFrameConstantBuffer();

		PerFrameConstantBuffer l_PerFrameConstantBuffer = g_Engine->Get<RenderingContextService>()->GetPerFrameConstantBuffer();
		l_PerFrameConstantBuffer.posWSNormalizer = m_irradianceVolumeRange;

		GIConstantBuffer l_GIConstantBuffer;

		auto l_probeInfo = GIDataLoader::GetProbeInfo();
		l_GIConstantBuffer.probeCount = l_probeInfo.probeCount;
		l_GIConstantBuffer.probeRange = l_probeInfo.probeRange;
		l_GIConstantBuffer.workload.x = (float)m_surfelGPUBufferComp->m_ElementCount;
		l_GIConstantBuffer.workload.y = (float)m_brickGPUBufferComp->m_ElementCount;
		l_GIConstantBuffer.workload.z = (float)m_probeGPUBufferComp->m_ElementCount;
		l_GIConstantBuffer.irradianceVolumeOffset = m_irradianceVolumePosOffset;
		l_GIConstantBuffer.probeCount.w = m_minProbePos.x;
		l_GIConstantBuffer.probeRange.w = m_minProbePos.y;
		l_GIConstantBuffer.irradianceVolumeOffset.w = m_minProbePos.z;

		l_renderingServer->Upload(l_PerFrameCBufferGPUBufferComp, &l_PerFrameConstantBuffer);
		l_renderingServer->Upload(l_GIGPUBufferComp, &l_GIConstantBuffer);

		generateSkyRadiance();
		generateSkyIrradiance();
		litSurfels();
		litBricks();
		litProbes();
		generateIrradianceVolume();

		l_renderingServer->ExecuteCommandList(m_skyRadianceRenderPassComp, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(m_skyIrradianceRenderPassComp, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(m_surfelRenderPassComp, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(m_brickRenderPassComp, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(m_probeRenderPassComp, GPUEngineType::Graphics);
		l_renderingServer->ExecuteCommandList(m_irradianceVolumeRenderPassComp, GPUEngineType::Graphics);	
	}

	return true;
}

bool GIResolvePass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->Delete(m_skyRadianceRenderPassComp);
	l_renderingServer->Delete(m_skyIrradianceRenderPassComp);
	l_renderingServer->Delete(m_surfelRenderPassComp);
	l_renderingServer->Delete(m_brickRenderPassComp);
	l_renderingServer->Delete(m_probeRenderPassComp);
	l_renderingServer->Delete(m_irradianceVolumeRenderPassComp);

	return true;
}

RenderPassComponent* GIResolvePass::GetRenderPassComp()
{
	return m_surfelRenderPassComp;
}

ShaderProgramComponent* GIResolvePass::GetSPC()
{
	return m_surfelSPC;
}

GPUResourceComponent* GIResolvePass::GetProbeVolume()
{
	if (m_probeVolume)
	{
		return m_probeVolume;
	}
	else
	{
		return nullptr;
	}
}

GPUResourceComponent* GIResolvePass::GetIrradianceVolume()
{
	if (m_irradianceVolume)
	{
		return m_irradianceVolume;
	}
	else
	{
		return nullptr;
	}
}