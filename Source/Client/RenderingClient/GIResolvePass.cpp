#include "GIResolvePass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "GIDataLoader.h"
#include "SunShadowBlurEvenPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine* g_Engine;

using namespace DefaultGPUBuffers;

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

	std::function<void()> f_sceneLoadingFinishCallback;
	std::function<void()> f_sceneLoadingStartCallback;
	std::function<void()> f_reloadGIData;
	bool m_needToReloadGIData = false;

	bool m_GIDataLoaded = false;
}

bool GIResolvePass::InitializeGPUBuffers()
{
	auto l_surfels = GIDataLoader::GetSurfels();

	if (l_surfels.size())
	{
		auto l_GIResolvePassInitializeGPUBuffersTask = g_Engine->getTaskSystem()->Submit("GIResolvePassInitializeGPUBuffersTask", 2, nullptr,
			[&]() {
				m_surfelGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("SurfelGPUBuffer/");
				m_surfelGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_surfelGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_surfelGPUBufferComp->m_ElementCount = l_surfels.size();
				m_surfelGPUBufferComp->m_ElementSize = sizeof(Surfel);
				m_surfelGPUBufferComp->m_InitialData = &l_surfels[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_surfelGPUBufferComp);

				m_surfelIrradianceGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("SurfelIrradianceGPUBuffer/");
				m_surfelIrradianceGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_surfelIrradianceGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_surfelIrradianceGPUBufferComp->m_ElementCount = l_surfels.size();
				m_surfelIrradianceGPUBufferComp->m_ElementSize = sizeof(Vec4);

				g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_surfelIrradianceGPUBufferComp);

				auto l_bricks = GIDataLoader::GetBricks();

				auto l_bricksCount = l_bricks.size();

				auto l_min = InnoMath::maxVec4<float>;
				auto l_max = InnoMath::minVec4<float>;

				for (size_t i = 0; i < l_bricksCount; i++)
				{
					l_min = InnoMath::elementWiseMin(l_min, l_bricks[i].boundBox.m_boundMin);
					l_max = InnoMath::elementWiseMax(l_max, l_bricks[i].boundBox.m_boundMax);
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

				m_brickGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("BrickGPUBuffer/");
				m_brickGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_brickGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickGPUBufferComp->m_ElementCount = l_bricks.size();
				m_brickGPUBufferComp->m_ElementSize = sizeof(uint32_t) * 2;
				m_brickGPUBufferComp->m_InitialData = &l_brickConstantBuffer[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_brickGPUBufferComp);

				m_brickIrradianceGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("BrickIrradianceGPUBuffer/");
				m_brickIrradianceGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_brickIrradianceGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickIrradianceGPUBufferComp->m_ElementCount = l_bricks.size();
				m_brickIrradianceGPUBufferComp->m_ElementSize = sizeof(Vec4);

				g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_brickIrradianceGPUBufferComp);

				auto l_brickFactors = GIDataLoader::GetBrickFactors();

				m_brickFactorGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("BrickFactorGPUBuffer/");
				m_brickFactorGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_brickFactorGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickFactorGPUBufferComp->m_ElementCount = l_brickFactors.size();
				m_brickFactorGPUBufferComp->m_ElementSize = sizeof(BrickFactor);
				m_brickFactorGPUBufferComp->m_InitialData = &l_brickFactors[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_brickFactorGPUBufferComp);

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

				m_probeGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("ProbeGPUBuffer/");
				m_probeGPUBufferComp->m_CPUAccessibility = Accessibility::Immutable;
				m_probeGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
				m_probeGPUBufferComp->m_ElementCount = l_probes.size();
				m_probeGPUBufferComp->m_ElementSize = sizeof(Probe);
				m_probeGPUBufferComp->m_InitialData = &l_probes[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_probeGPUBufferComp);

				auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

				m_probeVolume = g_Engine->getRenderingServer()->AddTextureComponent("ProbeVolume/");
				m_probeVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

				m_probeVolume->m_TextureDesc.Width = (uint32_t)l_probeIndex.x + 1;
				m_probeVolume->m_TextureDesc.Height = (uint32_t)l_probeIndex.y + 1;
				m_probeVolume->m_TextureDesc.DepthOrArraySize = ((uint32_t)l_probeIndex.z + 1) * 6;
				m_probeVolume->m_TextureDesc.Usage = TextureUsage::Sample;
				m_probeVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
				m_probeVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

				g_Engine->getRenderingServer()->InitializeTextureComponent(m_probeVolume);

				m_irradianceVolume = g_Engine->getRenderingServer()->AddTextureComponent("IrradianceVolume/");
				m_irradianceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

				m_irradianceVolume->m_TextureDesc.Width = 64;
				m_irradianceVolume->m_TextureDesc.Height = 32;
				m_irradianceVolume->m_TextureDesc.DepthOrArraySize = 64 * 6;
				m_irradianceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
				m_irradianceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
				m_irradianceVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

				g_Engine->getRenderingServer()->InitializeTextureComponent(m_irradianceVolume);

				m_GIDataLoaded = true;
			});

		l_GIResolvePassInitializeGPUBuffersTask.m_Future->Get();
	}

	return true;
}

bool GIResolvePass::DeleteGPUBuffers()
{
	auto l_GIResolvePassDeleteGPUBuffersTask = g_Engine->getTaskSystem()->Submit("GIResolvePassDeleteGPUBuffersTask", 2, nullptr,
		[&]() {
			if (m_surfelGPUBufferComp)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferComponent(m_surfelGPUBufferComp);
			}
			if (m_surfelIrradianceGPUBufferComp)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferComponent(m_surfelIrradianceGPUBufferComp);
			}
			if (m_brickGPUBufferComp)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferComponent(m_brickGPUBufferComp);
			}
			if (m_brickIrradianceGPUBufferComp)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferComponent(m_brickIrradianceGPUBufferComp);
			}
			if (m_brickFactorGPUBufferComp)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferComponent(m_brickFactorGPUBufferComp);
			}
			if (m_probeGPUBufferComp)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferComponent(m_probeGPUBufferComp);
			}
			if (m_probeVolume)
			{
				g_Engine->getRenderingServer()->DeleteTextureComponent(m_probeVolume);
			}

			m_GIDataLoaded = false;
		});

	l_GIResolvePassDeleteGPUBuffersTask.m_Future->Get();

	return true;
}

bool GIResolvePass::Setup()
{
	f_reloadGIData = [&]() { m_needToReloadGIData = true; };
	g_Engine->getEventSystem()->addButtonStateCallback(ButtonState{ INNO_KEY_B, true }, ButtonEvent{ EventLifeTime::OneShot, &f_reloadGIData });

	setupSky();
	setupSurfels();
	setupBricks();
	setupProbes();
	setupIrradianceVolume();

	f_sceneLoadingFinishCallback = []() { InitializeGPUBuffers(); };
	f_sceneLoadingStartCallback = []() { DeleteGPUBuffers(); };

	g_Engine->getSceneSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);
	g_Engine->getSceneSystem()->addSceneLoadingStartCallback(&f_sceneLoadingStartCallback);

	return true;
}

bool GIResolvePass::Initialize()
{
	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_skyRadianceSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_skyRadianceRenderPassComp);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_skyIrradianceSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_skyIrradianceRenderPassComp);

	g_Engine->getRenderingServer()->InitializeTextureComponent(m_skyRadianceVolume);
	g_Engine->getRenderingServer()->InitializeGPUBufferComponent(m_skyIrradianceGPUBufferComp);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_surfelSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_surfelRenderPassComp);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_brickSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_brickRenderPassComp);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_probeSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_probeRenderPassComp);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_irradianceVolumeSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassComponent(m_irradianceVolumeRenderPassComp);
	g_Engine->getRenderingServer()->InitializeSamplerComponent(m_irradianceVolumeSamplerComp);

	return true;
}

bool GIResolvePass::setupSky()
{
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_skyRadianceSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveSkyRadiancePass/");
	m_skyRadianceSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSkyRadiancePass.comp/";

	m_skyRadianceRenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("GIResolveSkyRadiancePass/");

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
	m_skyIrradianceSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveSkyIrradiancePass/");
	m_skyIrradianceSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSkyIrradiancePass.comp/";

	m_skyIrradianceRenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("GIResolveSkyIrradiancePass/");

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
	m_skyRadianceVolume = g_Engine->getRenderingServer()->AddTextureComponent("SkyRadianceVolume/");
	m_skyRadianceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_skyRadianceVolume->m_TextureDesc.Width = 8;
	m_skyRadianceVolume->m_TextureDesc.Height = 8;
	m_skyRadianceVolume->m_TextureDesc.DepthOrArraySize = 6;
	m_skyRadianceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_skyRadianceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_skyRadianceVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	m_skyIrradianceGPUBufferComp = g_Engine->getRenderingServer()->AddGPUBufferComponent("SkyIrradianceGPUBuffer/");
	m_skyIrradianceGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceGPUBufferComp->m_ElementCount = 6;
	m_skyIrradianceGPUBufferComp->m_ElementSize = sizeof(Vec4);

	return true;
}

bool GIResolvePass::setupSurfels()
{
	m_surfelSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveSurfelPass/");
	m_surfelSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSurfelPass.comp/";

	m_surfelRenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("GIResolveSurfelPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

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
	m_brickSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveBrickPass/");
	m_brickSPC->m_ShaderFilePaths.m_CSPath = "GIResolveBrickPass.comp/";

	m_brickRenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("GIResolveBrickPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

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
	m_probeSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveProbePass/");
	m_probeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveProbePass.comp/";

	m_probeRenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("GIResolveProbePass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

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
	m_irradianceVolumeSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveIrradianceVolumePass.comp/";

	m_irradianceVolumeRenderPassComp = g_Engine->getRenderingServer()->AddRenderPassComponent("GIResolveIrradianceVolumePass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

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
	m_irradianceVolumeSamplerComp = g_Engine->getRenderingServer()->AddSamplerComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Border;
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Border;
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_WrapMethodW = TextureWrapMethod::Border;
	m_irradianceVolumeSamplerComp->m_SamplerDesc.m_BorderColor[3] = 1.0f;

	return true;
}

bool GIResolvePass::generateSkyRadiance()
{
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_skyRadianceRenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_skyRadianceRenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_skyRadianceRenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_skyRadianceRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_skyRadianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_skyRadianceRenderPassComp, 1, 1, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_skyRadianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_skyRadianceRenderPassComp);

	return true;
}

bool GIResolvePass::generateSkyIrradiance()
{
	g_Engine->getRenderingServer()->CommandListBegin(m_skyIrradianceRenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_skyIrradianceRenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_skyIrradianceRenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_skyIrradianceRenderPassComp, 1, 1, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyRadianceVolume, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_skyIrradianceRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_skyIrradianceRenderPassComp);

	return true;
}

bool GIResolvePass::litSurfels()
{
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_CSMGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::CSM);
	auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = std::ceil((double)m_surfelGPUBufferComp->m_ElementCount / (double)(l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide));
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_surfelLitWorkload;
	l_surfelLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_surfelLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &l_surfelLitWorkload, 2, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_surfelRenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_surfelRenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_surfelRenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_CSMGPUBufferComp, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelGPUBufferComp, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6);

	g_Engine->getRenderingServer()->Dispatch(m_surfelRenderPassComp, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelGPUBufferComp, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_surfelRenderPassComp, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6);

	g_Engine->getRenderingServer()->CommandListEnd(m_surfelRenderPassComp);

	return true;
}

bool GIResolvePass::litBricks()
{
	auto l_dispatchParamsGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_brickGPUBufferComp->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_brickLitWorkload;
	l_brickLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_brickLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &l_brickLitWorkload, 3, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_brickRenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_brickRenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_brickRenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickGPUBufferComp, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 4, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_brickRenderPassComp, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickGPUBufferComp, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_surfelIrradianceGPUBufferComp, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_brickRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 4, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_brickRenderPassComp);

	return true;
}

bool GIResolvePass::litProbes()
{
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_probeGPUBufferComp->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_probeLitWorkload;
	l_probeLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_probeLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &l_probeLitWorkload, 4, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_probeRenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_probeRenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_probeRenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeGPUBufferComp, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickFactorGPUBufferComp, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeVolume, 6, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_probeRenderPassComp, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeGPUBufferComp, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickFactorGPUBufferComp, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_brickIrradianceGPUBufferComp, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRenderPassComp, ShaderStage::Compute, m_probeVolume, 6, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_probeRenderPassComp);

	return true;
}

bool GIResolvePass::generateIrradianceVolume()
{
	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);

	auto l_numThreadsX = 64;
	auto l_numThreadsY = 32;
	auto l_numThreadsZ = 64;

	DispatchParamsConstantBuffer l_irradianceVolumeLitWorkload;
	l_irradianceVolumeLitWorkload.numThreadGroups = TVec4<uint32_t>(8, 4, 8, 0);
	l_irradianceVolumeLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &l_irradianceVolumeLitWorkload, 5, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_irradianceVolumeRenderPassComp, 0);
	g_Engine->getRenderingServer()->BindRenderPassComponent(m_irradianceVolumeRenderPassComp);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_irradianceVolumeRenderPassComp);

	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_irradianceVolumeSamplerComp, 6);

	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, l_GIGPUBufferComp, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_probeVolume, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_irradianceVolume, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 5, Accessibility::ReadWrite);

	// @TODO: Buggy on OpenGL + Nvidia
	g_Engine->getRenderingServer()->Dispatch(m_irradianceVolumeRenderPassComp, 8, 4, 8);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_probeVolume, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_irradianceVolume, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_irradianceVolumeRenderPassComp, ShaderStage::Compute, m_skyIrradianceGPUBufferComp, 5, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_irradianceVolumeRenderPassComp);

	return true;
}

bool GIResolvePass::PrepareCommandList()
{
	if (m_needToReloadGIData)
	{
		GIDataLoader::ReloadGIData();
		m_needToReloadGIData = false;
		DeleteGPUBuffers();
		InitializeGPUBuffers();
	}

	if (m_GIDataLoaded)
	{
		auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
		auto l_GIGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::GI);
		auto l_cameraConstantBuffer = g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer();

		PerFrameConstantBuffer l_PerFrameConstantBuffer = g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer();
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

		g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_PerFrameCBufferGPUBufferComp, &l_PerFrameConstantBuffer);
		g_Engine->getRenderingServer()->UploadGPUBufferComponent(l_GIGPUBufferComp, &l_GIConstantBuffer);

		generateSkyRadiance();
		generateSkyIrradiance();
		litSurfels();
		litBricks();
		litProbes();
		generateIrradianceVolume();

		g_Engine->getRenderingServer()->ExecuteCommandList(m_skyRadianceRenderPassComp, GPUEngineType::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_skyIrradianceRenderPassComp, GPUEngineType::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_surfelRenderPassComp, GPUEngineType::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_brickRenderPassComp, GPUEngineType::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_probeRenderPassComp, GPUEngineType::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_irradianceVolumeRenderPassComp, GPUEngineType::Graphics);	
	}

	return true;
}

bool GIResolvePass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_skyRadianceRenderPassComp);
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_skyIrradianceRenderPassComp);
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_surfelRenderPassComp);
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_brickRenderPassComp);
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_probeRenderPassComp);
	g_Engine->getRenderingServer()->DeleteRenderPassComponent(m_irradianceVolumeRenderPassComp);

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