#include "VolumetricPass_Internal.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"

using namespace Inno;

namespace VolumetricPass
{
	SamplerComponent* m_SamplerComp;

	RenderPassComponent* m_froxelizationRenderPassComp;
	ShaderProgramComponent* m_froxelizationSPC;

	RenderPassComponent* m_visualizationRenderPassComp;
	ShaderProgramComponent* m_visualizationSPC;

	RenderPassComponent* m_irraidanceInjectionRenderPassComp;
	ShaderProgramComponent* m_irraidanceInjectionSPC;

	RenderPassComponent* m_rayMarchingRenderPassComp;
	ShaderProgramComponent* m_rayMarchingSPC;

	TextureComponent* m_irraidanceInjectionResult;
	TextureComponent* m_rayMarchingResult_A;
	TextureComponent* m_rayMarchingResult_B;

	CommandListComponent* m_froxelizationCommandListComp;
	CommandListComponent* m_irraidanceInjectionCommandListComp;
	CommandListComponent* m_rayMarchingCommandListComp;
	CommandListComponent* m_visualizationCommandListComp;

	TVec4<uint32_t> m_voxelizationResolution = TVec4<uint32_t>(160, 90, 64, 0);
	bool m_isPassA = true;
} // namespace VolumetricPass

bool VolumetricPass::Setup()
{

	m_SamplerComp = g_Engine->Get<SamplerResourceService>()->Add("VolumetricPass");

	setupGeometryProcessPass();
	setupIrradianceInjectionPass();
	setupRayMarchingPass();
	setupVisualizationPass();

	// Add command list components for each render pass
	m_froxelizationCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/Froxelization/Graphics");
	m_froxelizationCommandListComp->m_Type = GPUEngineType::Graphics;

	m_irraidanceInjectionCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/IrraidanceInjection/Compute");
	m_irraidanceInjectionCommandListComp->m_Type = GPUEngineType::Compute;

	m_rayMarchingCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/RayMarching/Compute");
	m_rayMarchingCommandListComp->m_Type = GPUEngineType::Compute;

	m_visualizationCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/Visualization/Graphics");
	m_visualizationCommandListComp->m_Type = GPUEngineType::Graphics;

	////
	auto l_textureDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc().m_RenderTargetDesc;

	l_textureDesc.Sampler = TextureSampler::Sampler3D;
	l_textureDesc.Usage = TextureUsage::Sample;
	l_textureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_textureDesc.Width = m_voxelizationResolution.x;
	l_textureDesc.Height = m_voxelizationResolution.y;
	l_textureDesc.DepthOrArraySize = m_voxelizationResolution.z;
	l_textureDesc.PixelDataType = TexturePixelDataType::Float32;

	m_irraidanceInjectionResult = g_Engine->Get<TextureResourceService>()->Add("VolumetricIrraidanceInjectionResult");
	m_irraidanceInjectionResult->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_A = g_Engine->Get<TextureResourceService>()->Add("VolumetricRayMarchingResult_A");
	m_rayMarchingResult_A->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_B = g_Engine->Get<TextureResourceService>()->Add("VolumetricRayMarchingResult_B");
	m_rayMarchingResult_B->m_TextureDesc = l_textureDesc;

	return true;
}

bool VolumetricPass::Initialize()
{

	g_Engine->Get<SamplerResourceService>()->Initialize(m_SamplerComp);

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_froxelizationSPC);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_froxelizationRenderPassComp);

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_irraidanceInjectionSPC);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_irraidanceInjectionRenderPassComp);

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_rayMarchingSPC);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_rayMarchingRenderPassComp);

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_visualizationSPC);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_visualizationRenderPassComp);

	// Initialize command list components
	g_Engine->Get<CommandListResourceService>()->Initialize(m_froxelizationCommandListComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_irraidanceInjectionCommandListComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_rayMarchingCommandListComp);
	g_Engine->Get<CommandListResourceService>()->Initialize(m_visualizationCommandListComp);

	g_Engine->Get<TextureResourceService>()->Initialize(m_irraidanceInjectionResult);
	g_Engine->Get<TextureResourceService>()->Initialize(m_rayMarchingResult_A);
	g_Engine->Get<TextureResourceService>()->Initialize(m_rayMarchingResult_B);

	return true;
}

bool VolumetricPass::Terminate()
{

	g_Engine->Get<RenderPassResourceService>()->Delete(m_froxelizationRenderPassComp);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_irraidanceInjectionRenderPassComp);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_rayMarchingRenderPassComp);
	g_Engine->Get<RenderPassResourceService>()->Delete(m_visualizationRenderPassComp);

	return true;
}

GPUResourceComponent* VolumetricPass::GetRayMarchingResult()
{
	if (m_isPassA)
	{
		return m_rayMarchingResult_B;
	}
	else
	{
		return m_rayMarchingResult_A;
	}
}

GPUResourceComponent* VolumetricPass::GetVisualizationResult()
{
	return nullptr;
}
