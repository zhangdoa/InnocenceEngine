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

	RenderPassDataComponent* m_skyRadianceRPDC;
	ShaderProgramComponent* m_skyRadianceSPC;

	RenderPassDataComponent* m_skyIrradianceRPDC;
	ShaderProgramComponent* m_skyIrradianceSPC;

	RenderPassDataComponent* m_surfelRPDC = 0;
	ShaderProgramComponent* m_surfelSPC = 0;

	RenderPassDataComponent* m_brickRPDC = 0;
	ShaderProgramComponent* m_brickSPC = 0;

	RenderPassDataComponent* m_probeRPDC = 0;
	ShaderProgramComponent* m_probeSPC = 0;

	RenderPassDataComponent* m_irradianceVolumeRPDC = 0;
	ShaderProgramComponent* m_irradianceVolumeSPC = 0;
	SamplerDataComponent* m_irradianceVolumeSDC = 0;

	TextureDataComponent* m_skyRadianceVolume = 0;
	GPUBufferDataComponent* m_skyIrradianceGBDC = 0;

	GPUBufferDataComponent* m_surfelGBDC = 0;
	GPUBufferDataComponent* m_surfelIrradianceGBDC = 0;
	GPUBufferDataComponent* m_brickGBDC = 0;
	GPUBufferDataComponent* m_brickIrradianceGBDC = 0;
	GPUBufferDataComponent* m_brickFactorGBDC = 0;
	GPUBufferDataComponent* m_probeGBDC = 0;
	TextureDataComponent* m_probeVolume = 0;
	TextureDataComponent* m_irradianceVolume = 0;

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
				m_surfelGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("SurfelGPUBuffer/");
				m_surfelGBDC->m_CPUAccessibility = Accessibility::Immutable;
				m_surfelGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
				m_surfelGBDC->m_ElementCount = l_surfels.size();
				m_surfelGBDC->m_ElementSize = sizeof(Surfel);
				m_surfelGBDC->m_InitialData = &l_surfels[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_surfelGBDC);

				m_surfelIrradianceGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("SurfelIrradianceGPUBuffer/");
				m_surfelIrradianceGBDC->m_CPUAccessibility = Accessibility::Immutable;
				m_surfelIrradianceGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
				m_surfelIrradianceGBDC->m_ElementCount = l_surfels.size();
				m_surfelIrradianceGBDC->m_ElementSize = sizeof(Vec4);

				g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_surfelIrradianceGBDC);

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

				m_brickGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("BrickGPUBuffer/");
				m_brickGBDC->m_CPUAccessibility = Accessibility::Immutable;
				m_brickGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickGBDC->m_ElementCount = l_bricks.size();
				m_brickGBDC->m_ElementSize = sizeof(uint32_t) * 2;
				m_brickGBDC->m_InitialData = &l_brickConstantBuffer[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickGBDC);

				m_brickIrradianceGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("BrickIrradianceGPUBuffer/");
				m_brickIrradianceGBDC->m_CPUAccessibility = Accessibility::Immutable;
				m_brickIrradianceGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickIrradianceGBDC->m_ElementCount = l_bricks.size();
				m_brickIrradianceGBDC->m_ElementSize = sizeof(Vec4);

				g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickIrradianceGBDC);

				auto l_brickFactors = GIDataLoader::GetBrickFactors();

				m_brickFactorGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("BrickFactorGPUBuffer/");
				m_brickFactorGBDC->m_CPUAccessibility = Accessibility::Immutable;
				m_brickFactorGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
				m_brickFactorGBDC->m_ElementCount = l_brickFactors.size();
				m_brickFactorGBDC->m_ElementSize = sizeof(BrickFactor);
				m_brickFactorGBDC->m_InitialData = &l_brickFactors[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_brickFactorGBDC);

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

				m_probeGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("ProbeGPUBuffer/");
				m_probeGBDC->m_CPUAccessibility = Accessibility::Immutable;
				m_probeGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
				m_probeGBDC->m_ElementCount = l_probes.size();
				m_probeGBDC->m_ElementSize = sizeof(Probe);
				m_probeGBDC->m_InitialData = &l_probes[0];

				g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_probeGBDC);

				auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

				m_probeVolume = g_Engine->getRenderingServer()->AddTextureDataComponent("ProbeVolume/");
				m_probeVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

				m_probeVolume->m_TextureDesc.Width = (uint32_t)l_probeIndex.x + 1;
				m_probeVolume->m_TextureDesc.Height = (uint32_t)l_probeIndex.y + 1;
				m_probeVolume->m_TextureDesc.DepthOrArraySize = ((uint32_t)l_probeIndex.z + 1) * 6;
				m_probeVolume->m_TextureDesc.Usage = TextureUsage::Sample;
				m_probeVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
				m_probeVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

				g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_probeVolume);

				m_irradianceVolume = g_Engine->getRenderingServer()->AddTextureDataComponent("IrradianceVolume/");
				m_irradianceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

				m_irradianceVolume->m_TextureDesc.Width = 64;
				m_irradianceVolume->m_TextureDesc.Height = 32;
				m_irradianceVolume->m_TextureDesc.DepthOrArraySize = 64 * 6;
				m_irradianceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
				m_irradianceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
				m_irradianceVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

				g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_irradianceVolume);

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
			if (m_surfelGBDC)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_surfelGBDC);
			}
			if (m_surfelIrradianceGBDC)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_surfelIrradianceGBDC);
			}
			if (m_brickGBDC)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickGBDC);
			}
			if (m_brickIrradianceGBDC)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickIrradianceGBDC);
			}
			if (m_brickFactorGBDC)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_brickFactorGBDC);
			}
			if (m_probeGBDC)
			{
				g_Engine->getRenderingServer()->DeleteGPUBufferDataComponent(m_probeGBDC);
			}
			if (m_probeVolume)
			{
				g_Engine->getRenderingServer()->DeleteTextureDataComponent(m_probeVolume);
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
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_skyRadianceRPDC);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_skyIrradianceSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_skyIrradianceRPDC);

	g_Engine->getRenderingServer()->InitializeTextureDataComponent(m_skyRadianceVolume);
	g_Engine->getRenderingServer()->InitializeGPUBufferDataComponent(m_skyIrradianceGBDC);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_surfelSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_surfelRPDC);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_brickSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_brickRPDC);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_probeSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_probeRPDC);

	g_Engine->getRenderingServer()->InitializeShaderProgramComponent(m_irradianceVolumeSPC);
	g_Engine->getRenderingServer()->InitializeRenderPassDataComponent(m_irradianceVolumeRPDC);
	g_Engine->getRenderingServer()->InitializeSamplerDataComponent(m_irradianceVolumeSDC);

	return true;
}

bool GIResolvePass::setupSky()
{
	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_skyRadianceSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveSkyRadiancePass/");
	m_skyRadianceSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSkyRadiancePass.comp/";

	m_skyRadianceRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("GIResolveSkyRadiancePass/");

	m_skyRadianceRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs.resize(2);
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Image;
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_skyRadianceRPDC->m_ResourceBindingLayoutDescs[1].m_IndirectBinding = true;

	m_skyRadianceRPDC->m_ShaderProgram = m_skyRadianceSPC;

	////
	m_skyIrradianceSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveSkyIrradiancePass/");
	m_skyIrradianceSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSkyIrradiancePass.comp/";

	m_skyIrradianceRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("GIResolveSkyIrradiancePass/");

	m_skyIrradianceRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs.resize(2);
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Image;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[0].m_BindingAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[0].m_IndirectBinding = true;

	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceRPDC->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_skyIrradianceRPDC->m_ShaderProgram = m_skyIrradianceSPC;

	////
	m_skyRadianceVolume = g_Engine->getRenderingServer()->AddTextureDataComponent("SkyRadianceVolume/");
	m_skyRadianceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_skyRadianceVolume->m_TextureDesc.Width = 8;
	m_skyRadianceVolume->m_TextureDesc.Height = 8;
	m_skyRadianceVolume->m_TextureDesc.DepthOrArraySize = 6;
	m_skyRadianceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_skyRadianceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_skyRadianceVolume->m_TextureDesc.PixelDataFormat = TexturePixelDataFormat::RGBA;

	m_skyIrradianceGBDC = g_Engine->getRenderingServer()->AddGPUBufferDataComponent("SkyIrradianceGPUBuffer/");
	m_skyIrradianceGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_skyIrradianceGBDC->m_ElementCount = 6;
	m_skyIrradianceGBDC->m_ElementSize = sizeof(Vec4);

	return true;
}

bool GIResolvePass::setupSurfels()
{
	m_surfelSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveSurfelPass/");
	m_surfelSPC->m_ShaderFilePaths.m_CSPath = "GIResolveSurfelPass.comp/";

	m_surfelRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("GIResolveSurfelPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_surfelRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_surfelRPDC->m_ResourceBindingLayoutDescs.resize(7);
	m_surfelRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_surfelRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 5;

	m_surfelRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 6;

	m_surfelRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 8;

	m_surfelRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;

	m_surfelRPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;

	m_surfelRPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_surfelRPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_surfelRPDC->m_ShaderProgram = m_surfelSPC;

	return true;
}

bool GIResolvePass::setupBricks()
{
	m_brickSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveBrickPass/");
	m_brickSPC->m_ShaderFilePaths.m_CSPath = "GIResolveBrickPass.comp/";

	m_brickRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("GIResolveBrickPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_brickRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_brickRPDC->m_ResourceBindingLayoutDescs.resize(5);
	m_brickRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_brickRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 6;

	m_brickRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_brickRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 8;

	m_brickRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRPDC->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_brickRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;

	m_brickRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRPDC->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_brickRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;

	m_brickRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_brickRPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_brickRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_brickRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;

	m_brickRPDC->m_ShaderProgram = m_brickSPC;

	return true;
}

bool GIResolvePass::setupProbes()
{
	m_probeSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveProbePass/");
	m_probeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveProbePass.comp/";

	m_probeRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("GIResolveProbePass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_probeRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_probeRPDC->m_ResourceBindingLayoutDescs.resize(7);
	m_probeRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_probeRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_probeRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_probeRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 6;

	m_probeRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_probeRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 8;

	m_probeRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRPDC->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_probeRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;

	m_probeRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_probeRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;

	m_probeRPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_probeRPDC->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_probeRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 2;

	m_probeRPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_probeRPDC->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_probeRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 1;
	m_probeRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 3;
	m_probeRPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_probeRPDC->m_ShaderProgram = m_probeSPC;

	return true;
}

bool GIResolvePass::setupIrradianceVolume()
{
	m_irradianceVolumeSPC = g_Engine->getRenderingServer()->AddShaderProgramComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSPC->m_ShaderFilePaths.m_CSPath = "GIResolveIrradianceVolumePass.comp/";

	m_irradianceVolumeRPDC = g_Engine->getRenderingServer()->AddRenderPassDataComponent("GIResolveIrradianceVolumePass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_irradianceVolumeRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs.resize(7);
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 6;

	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 8;

	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Buffer;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;

	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Sampler;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 3;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_irradianceVolumeRPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_irradianceVolumeRPDC->m_ShaderProgram = m_irradianceVolumeSPC;
	m_irradianceVolumeSDC = g_Engine->getRenderingServer()->AddSamplerDataComponent("GIResolveIrradianceVolumePass/");
	m_irradianceVolumeSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Border;
	m_irradianceVolumeSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Border;
	m_irradianceVolumeSDC->m_SamplerDesc.m_WrapMethodW = TextureWrapMethod::Border;
	m_irradianceVolumeSDC->m_SamplerDesc.m_BorderColor[3] = 1.0f;

	return true;
}

bool GIResolvePass::generateSkyRadiance()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_Engine->getRenderingServer()->CommandListBegin(m_skyRadianceRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_skyRadianceRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_skyRadianceRPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_skyRadianceRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_skyRadianceRPDC, ShaderStage::Compute, m_skyRadianceVolume, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_skyRadianceRPDC, 1, 1, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_skyRadianceRPDC, ShaderStage::Compute, m_skyRadianceVolume, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_skyRadianceRPDC);

	return true;
}

bool GIResolvePass::generateSkyIrradiance()
{
	g_Engine->getRenderingServer()->CommandListBegin(m_skyIrradianceRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_skyIrradianceRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_skyIrradianceRPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_skyIrradianceRPDC, ShaderStage::Compute, m_skyRadianceVolume, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_skyIrradianceRPDC, ShaderStage::Compute, m_skyIrradianceGBDC, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_skyIrradianceRPDC, 1, 1, 1);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_skyIrradianceRPDC, ShaderStage::Compute, m_skyRadianceVolume, 0, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_skyIrradianceRPDC, ShaderStage::Compute, m_skyIrradianceGBDC, 1, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_skyIrradianceRPDC);

	return true;
}

bool GIResolvePass::litSurfels()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);
	auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = std::ceil((double)m_surfelGBDC->m_ElementCount / (double)(l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide));
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)std::ceil(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_surfelLitWorkload;
	l_surfelLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_surfelLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_surfelLitWorkload, 2, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_surfelRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_surfelRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_surfelRPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRPDC, ShaderStage::Compute, l_CSMGBDC, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRPDC, ShaderStage::Compute, l_dispatchParamsGBDC, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRPDC, ShaderStage::Compute, l_GIGBDC, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRPDC, ShaderStage::Compute, m_surfelGBDC, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_surfelRPDC, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6);

	g_Engine->getRenderingServer()->Dispatch(m_surfelRPDC, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_surfelRPDC, ShaderStage::Compute, m_surfelGBDC, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_surfelRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_surfelRPDC, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6);

	g_Engine->getRenderingServer()->CommandListEnd(m_surfelRPDC);

	return true;
}

bool GIResolvePass::litBricks()
{
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_brickGBDC->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_brickLitWorkload;
	l_brickLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_brickLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_brickLitWorkload, 3, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_brickRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_brickRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_brickRPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_brickRPDC, ShaderStage::Compute, l_dispatchParamsGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRPDC, ShaderStage::Compute, l_GIGBDC, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRPDC, ShaderStage::Compute, m_brickGBDC, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_brickRPDC, ShaderStage::Compute, m_brickIrradianceGBDC, 4, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_brickRPDC, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_brickRPDC, ShaderStage::Compute, m_brickGBDC, 2, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_brickRPDC, ShaderStage::Compute, m_surfelIrradianceGBDC, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_brickRPDC, ShaderStage::Compute, m_brickIrradianceGBDC, 4, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_brickRPDC);

	return true;
}

bool GIResolvePass::litProbes()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);

	auto l_threadCountPerGroupPerSide = 8;
	auto l_totalThreadGroupsCount = (double)m_probeGBDC->m_ElementCount / (l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide * l_threadCountPerGroupPerSide);
	auto l_averangeThreadGroupsCountPerSide = (uint32_t)std::ceil(std::pow(l_totalThreadGroupsCount, 1.0 / 3.0));

	auto l_numThreadsX = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsY = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);
	auto l_numThreadsZ = (uint32_t)(l_averangeThreadGroupsCountPerSide * l_threadCountPerGroupPerSide);

	DispatchParamsConstantBuffer l_probeLitWorkload;
	l_probeLitWorkload.numThreadGroups = TVec4<uint32_t>(l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, 0);
	l_probeLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_probeLitWorkload, 4, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_probeRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_probeRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_probeRPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_probeRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRPDC, ShaderStage::Compute, l_dispatchParamsGBDC, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRPDC, ShaderStage::Compute, l_GIGBDC, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRPDC, ShaderStage::Compute, m_probeGBDC, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRPDC, ShaderStage::Compute, m_brickFactorGBDC, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRPDC, ShaderStage::Compute, m_brickIrradianceGBDC, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_probeRPDC, ShaderStage::Compute, m_probeVolume, 6, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->Dispatch(m_probeRPDC, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide, l_averangeThreadGroupsCountPerSide);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRPDC, ShaderStage::Compute, m_probeGBDC, 3, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRPDC, ShaderStage::Compute, m_brickFactorGBDC, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRPDC, ShaderStage::Compute, m_brickIrradianceGBDC, 5, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_probeRPDC, ShaderStage::Compute, m_probeVolume, 6, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_probeRPDC);

	return true;
}

bool GIResolvePass::generateIrradianceVolume()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);
	auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);

	auto l_numThreadsX = 64;
	auto l_numThreadsY = 32;
	auto l_numThreadsZ = 64;

	DispatchParamsConstantBuffer l_irradianceVolumeLitWorkload;
	l_irradianceVolumeLitWorkload.numThreadGroups = TVec4<uint32_t>(8, 4, 8, 0);
	l_irradianceVolumeLitWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_irradianceVolumeLitWorkload, 5, 1);

	g_Engine->getRenderingServer()->CommandListBegin(m_irradianceVolumeRPDC, 0);
	g_Engine->getRenderingServer()->BindRenderPassDataComponent(m_irradianceVolumeRPDC);
	g_Engine->getRenderingServer()->CleanRenderTargets(m_irradianceVolumeRPDC);

	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, m_irradianceVolumeSDC, 6);

	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, l_dispatchParamsGBDC, 1, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, l_GIGBDC, 2, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, m_probeVolume, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, m_irradianceVolume, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->BindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, m_skyIrradianceGBDC, 5, Accessibility::ReadWrite);

	// @TODO: Buggy on OpenGL + Nvidia
	g_Engine->getRenderingServer()->Dispatch(m_irradianceVolumeRPDC, 8, 4, 8);

	g_Engine->getRenderingServer()->UnbindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, m_probeVolume, 3, Accessibility::ReadOnly);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, m_irradianceVolume, 4, Accessibility::ReadWrite);
	g_Engine->getRenderingServer()->UnbindGPUResource(m_irradianceVolumeRPDC, ShaderStage::Compute, m_skyIrradianceGBDC, 5, Accessibility::ReadWrite);

	g_Engine->getRenderingServer()->CommandListEnd(m_irradianceVolumeRPDC);

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
		auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
		auto l_GIGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::GI);
		auto l_cameraConstantBuffer = g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer();

		PerFrameConstantBuffer l_PerFrameConstantBuffer = g_Engine->getRenderingFrontend()->getPerFrameConstantBuffer();
		l_PerFrameConstantBuffer.posWSNormalizer = m_irradianceVolumeRange;

		GIConstantBuffer l_GIConstantBuffer;

		auto l_probeInfo = GIDataLoader::GetProbeInfo();
		l_GIConstantBuffer.probeCount = l_probeInfo.probeCount;
		l_GIConstantBuffer.probeRange = l_probeInfo.probeRange;
		l_GIConstantBuffer.workload.x = (float)m_surfelGBDC->m_ElementCount;
		l_GIConstantBuffer.workload.y = (float)m_brickGBDC->m_ElementCount;
		l_GIConstantBuffer.workload.z = (float)m_probeGBDC->m_ElementCount;
		l_GIConstantBuffer.irradianceVolumeOffset = m_irradianceVolumePosOffset;
		l_GIConstantBuffer.probeCount.w = m_minProbePos.x;
		l_GIConstantBuffer.probeRange.w = m_minProbePos.y;
		l_GIConstantBuffer.irradianceVolumeOffset.w = m_minProbePos.z;

		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_PerFrameCBufferGBDC, &l_PerFrameConstantBuffer);
		g_Engine->getRenderingServer()->UploadGPUBufferDataComponent(l_GIGBDC, &l_GIConstantBuffer);

		generateSkyRadiance();
		generateSkyIrradiance();
		litSurfels();
		litBricks();
		litProbes();
		generateIrradianceVolume();

		g_Engine->getRenderingServer()->ExecuteCommandList(m_skyRadianceRPDC, RenderPassUsage::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_skyIrradianceRPDC, RenderPassUsage::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_surfelRPDC, RenderPassUsage::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_brickRPDC, RenderPassUsage::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_probeRPDC, RenderPassUsage::Graphics);
		g_Engine->getRenderingServer()->ExecuteCommandList(m_irradianceVolumeRPDC, RenderPassUsage::Graphics);	
	}

	return true;
}

bool GIResolvePass::Terminate()
{
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_skyRadianceRPDC);
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_skyIrradianceRPDC);
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_surfelRPDC);
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_brickRPDC);
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_probeRPDC);
	g_Engine->getRenderingServer()->DeleteRenderPassDataComponent(m_irradianceVolumeRPDC);

	return true;
}

RenderPassDataComponent* GIResolvePass::GetRPDC()
{
	return m_surfelRPDC;
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