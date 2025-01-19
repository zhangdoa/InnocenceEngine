#include "VolumetricPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/RenderingContextService.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "LightCullingPass.h"

#include "../../Engine/Engine.h"

using namespace Inno;


using namespace DefaultGPUBuffers;

namespace VolumetricPass
{
	bool setupGeometryProcessPass();
	bool setupIrradianceInjectionPass();
	bool setupRayMarchingPass();
	bool setupVisualizationPass();

	bool froxelization();
	bool irraidanceInjection();
	bool rayMarching();
	bool visualization(GPUResourceComponent *input);

	SamplerComponent *m_SamplerComp;

	RenderPassComponent *m_froxelizationRenderPassComp;
	ShaderProgramComponent *m_froxelizationSPC;

	RenderPassComponent *m_visualizationRenderPassComp;
	ShaderProgramComponent *m_visualizationSPC;

	RenderPassComponent *m_irraidanceInjectionRenderPassComp;
	ShaderProgramComponent *m_irraidanceInjectionSPC;

	RenderPassComponent *m_rayMarchingRenderPassComp;
	ShaderProgramComponent *m_rayMarchingSPC;

	TextureComponent *m_irraidanceInjectionResult;
	TextureComponent *m_rayMarchingResult_A;
	TextureComponent *m_rayMarchingResult_B;

	TVec4<uint32_t> m_voxelizationResolution = TVec4<uint32_t>(160, 90, 64, 0);
	static bool m_isPassA = true;
} // namespace VolumetricPass

bool VolumetricPass::setupGeometryProcessPass()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_froxelizationSPC = l_renderingServer->AddShaderProgramComponent("VolumetricGeometryProcessPass/");

	m_froxelizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricGeometryProcessPass.vert/";
	m_froxelizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricGeometryProcessPass.geom/";
	m_froxelizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricGeometryProcessPass.frag/";

	m_froxelizationRenderPassComp = l_renderingServer->AddRenderPassComponent("VolumetricGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 2;
	l_RenderPassDesc.m_Resizable = false;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::Sample;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.Width = m_voxelizationResolution.x;
	l_RenderPassDesc.m_RenderTargetDesc.Height = m_voxelizationResolution.y;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = m_voxelizationResolution.z;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_voxelizationResolution.x;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_voxelizationResolution.y;

	m_froxelizationRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs.resize(5);
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_froxelizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_froxelizationRenderPassComp->m_ShaderProgram = m_froxelizationSPC;

	return true;
}

bool VolumetricPass::setupIrradianceInjectionPass()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_irraidanceInjectionSPC = l_renderingServer->AddShaderProgramComponent("VolumetricIrraidanceInjectionPass/");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricIrraidanceInjectionPass.comp/";

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_irraidanceInjectionRenderPassComp = l_renderingServer->AddRenderPassComponent("VolumetricIrraidanceInjectionPass/");

	m_irraidanceInjectionRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs.resize(10);
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 3;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 5;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 6;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 2;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 1;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[8].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[8].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 2;

	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Sampler;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 3;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 0;
	m_irraidanceInjectionRenderPassComp->m_ResourceBindingLayoutDescs[9].m_IndirectBinding = true;

	m_irraidanceInjectionRenderPassComp->m_ShaderProgram = m_irraidanceInjectionSPC;

	return true;
}

bool VolumetricPass::setupRayMarchingPass()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_rayMarchingSPC = l_renderingServer->AddShaderProgramComponent("VolumetricRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricRayMarchingPass.comp/";

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_rayMarchingRenderPassComp = l_renderingServer->AddRenderPassComponent("VolumetricRayMarchingPass/");

	m_rayMarchingRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs.resize(8);
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 6;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 3;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Sampler;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 3;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 0;
	m_rayMarchingRenderPassComp->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_rayMarchingRenderPassComp->m_ShaderProgram = m_rayMarchingSPC;

	return true;
}

bool VolumetricPass::setupVisualizationPass()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_visualizationSPC = l_renderingServer->AddShaderProgramComponent("VolumetricVisualizationPass/");

	m_visualizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricVisualizationPass.vert/";
	m_visualizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricVisualizationPass.frag/";

	m_visualizationRenderPassComp = l_renderingServer->AddRenderPassComponent("VolumetricVisualizationPass/");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->Get<RenderingConfigurationService>()->GetScreenResolution();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::ColorAttachment;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_viewportSize.x;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_viewportSize.y;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_viewportSize.x;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_viewportSize.y;

	m_visualizationRenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs.resize(5);
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Sampler;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_visualizationRenderPassComp->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_visualizationRenderPassComp->m_ShaderProgram = m_visualizationSPC;

	return true;
}

bool VolumetricPass::Setup()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	m_SamplerComp = l_renderingServer->AddSamplerComponent("VolumetricPass/");

	setupGeometryProcessPass();
	setupIrradianceInjectionPass();
	setupRayMarchingPass();
	setupVisualizationPass();

	////
	auto l_textureDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc().m_RenderTargetDesc;

	l_textureDesc.Sampler = TextureSampler::Sampler3D;
	l_textureDesc.Usage = TextureUsage::Sample;
	l_textureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_textureDesc.Width = m_voxelizationResolution.x;
	l_textureDesc.Height = m_voxelizationResolution.y;
	l_textureDesc.DepthOrArraySize = m_voxelizationResolution.z;
	l_textureDesc.PixelDataType = TexturePixelDataType::Float32;

	m_irraidanceInjectionResult = l_renderingServer->AddTextureComponent("VolumetricIrraidanceInjectionResult/");
	m_irraidanceInjectionResult->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_A = l_renderingServer->AddTextureComponent("VolumetricRayMarchingResult_A/");
	m_rayMarchingResult_A->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_B = l_renderingServer->AddTextureComponent("VolumetricRayMarchingResult_B/");
	m_rayMarchingResult_B->m_TextureDesc = l_textureDesc;

	return true;
}

bool VolumetricPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->InitializeSamplerComponent(m_SamplerComp);

	l_renderingServer->InitializeShaderProgramComponent(m_froxelizationSPC);
	l_renderingServer->InitializeRenderPassComponent(m_froxelizationRenderPassComp);

	l_renderingServer->InitializeShaderProgramComponent(m_irraidanceInjectionSPC);
	l_renderingServer->InitializeRenderPassComponent(m_irraidanceInjectionRenderPassComp);

	l_renderingServer->InitializeShaderProgramComponent(m_rayMarchingSPC);
	l_renderingServer->InitializeRenderPassComponent(m_rayMarchingRenderPassComp);

	l_renderingServer->InitializeShaderProgramComponent(m_visualizationSPC);
	l_renderingServer->InitializeRenderPassComponent(m_visualizationRenderPassComp);

	l_renderingServer->InitializeTextureComponent(m_irraidanceInjectionResult);
	l_renderingServer->InitializeTextureComponent(m_rayMarchingResult_A);
	l_renderingServer->InitializeTextureComponent(m_rayMarchingResult_B);

	return true;
}

bool VolumetricPass::froxelization()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);

	l_renderingServer->CommandListBegin(m_froxelizationRenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_froxelizationRenderPassComp);
	l_renderingServer->ClearRenderTargets(m_froxelizationRenderPassComp);

	l_renderingServer->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[0].m_Texture, 3);
	l_renderingServer->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[1].m_Texture, 4);

	auto &l_drawCallInfo = g_Engine->Get<RenderingContextService>()->GetDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::MainCamera);
		if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					l_renderingServer->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.meshConstantBufferIndex, 1);
					l_renderingServer->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.materialConstantBufferIndex, 1);

					l_renderingServer->DrawIndexedInstanced(m_froxelizationRenderPassComp, l_drawCallData.mesh);
				}
			}
		}
	}

	l_renderingServer->UnbindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[0].m_Texture, 3);
	l_renderingServer->UnbindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[1].m_Texture, 4);

	l_renderingServer->CommandListEnd(m_froxelizationRenderPassComp);

	return true;
}

bool VolumetricPass::irraidanceInjection()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PointLight);
	auto l_CSMGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::CSM);
	auto l_dispatchParamsGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = m_voxelizationResolution.z;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = (uint32_t)std::ceil((float)l_numThreadsZ / 8.0f);

	DispatchParamsConstantBuffer l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	l_renderingServer->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &l_irraidanceInjectionWorkload, 6, 1);

	l_renderingServer->CommandListBegin(m_irraidanceInjectionRenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_irraidanceInjectionRenderPassComp);
	l_renderingServer->ClearRenderTargets(m_irraidanceInjectionRenderPassComp);

	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_SamplerComp, 9);

	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_CSMGPUBufferComp, 2);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 3);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 4);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0].m_Texture, 5);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 6);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 7);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 8);

	l_renderingServer->Dispatch(m_irraidanceInjectionRenderPassComp, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 4);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0].m_Texture, 5);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 6);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 7);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 8);

	l_renderingServer->CommandListEnd(m_irraidanceInjectionRenderPassComp);

	return true;
}

bool VolumetricPass::rayMarching()
{	
	auto l_renderingServer = g_Engine->getRenderingServer();
	GPUResourceComponent *l_currentResultBinder;
	GPUResourceComponent *l_historyResultBinder;

	if (m_isPassA)
	{
		l_currentResultBinder = m_rayMarchingResult_A;
		l_historyResultBinder = m_rayMarchingResult_B;
		m_isPassA = false;
	}
	else
	{
		l_currentResultBinder = m_rayMarchingResult_B;
		l_historyResultBinder = m_rayMarchingResult_A;
		m_isPassA = true;
	}

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = 1;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = 1;

	DispatchParamsConstantBuffer l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	l_renderingServer->UploadGPUBufferComponent(l_dispatchParamsGPUBufferComp, &l_rayMarchingWorkload, 7, 1);

	l_renderingServer->CommandListBegin(m_rayMarchingRenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_rayMarchingRenderPassComp);
	l_renderingServer->ClearRenderTargets(m_rayMarchingRenderPassComp);

	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_SamplerComp, 7);

	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1);
	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 2);
	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0].m_Texture, 3);
	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[1].m_Texture, 4);
	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_historyResultBinder, 5);
	l_renderingServer->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_currentResultBinder, 6);

	l_renderingServer->Dispatch(m_rayMarchingRenderPassComp, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	l_renderingServer->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 2);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0].m_Texture, 3);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[1].m_Texture, 4);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_historyResultBinder, 5);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_currentResultBinder, 6);

	l_renderingServer->CommandListEnd(m_rayMarchingRenderPassComp);

	return true;
}

bool VolumetricPass::visualization(GPUResourceComponent *input)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	auto l_PerFrameCBufferGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGPUBufferComp = GetGPUBufferComponent(GPUBufferUsageType::Material);

	l_renderingServer->CommandListBegin(m_visualizationRenderPassComp, 0);
	l_renderingServer->BindRenderPassComponent(m_visualizationRenderPassComp);
	l_renderingServer->ClearRenderTargets(m_visualizationRenderPassComp);

	l_renderingServer->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, m_SamplerComp, 4);

	l_renderingServer->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, l_PerFrameCBufferGPUBufferComp, 0);
	l_renderingServer->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, input, 3);

	auto &l_drawCallInfo = g_Engine->Get<RenderingContextService>()->GetDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::MainCamera);
		if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					l_renderingServer->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.meshConstantBufferIndex, 1);
					l_renderingServer->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.materialConstantBufferIndex, 1);

					l_renderingServer->DrawIndexedInstanced(m_visualizationRenderPassComp, l_drawCallData.mesh);
				}
			}
		}
	}

	l_renderingServer->UnbindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, input, 3);

	l_renderingServer->CommandListEnd(m_visualizationRenderPassComp);

	return true;
}

bool VolumetricPass::Render(bool visualize)
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	froxelization();
	irraidanceInjection();
	rayMarching();

	if (visualize)
	{
		visualization(m_rayMarchingResult_A);
	}

	l_renderingServer->ExecuteCommandList(m_froxelizationRenderPassComp, GPUEngineType::Graphics);
	l_renderingServer->WaitCommandQueue(m_froxelizationRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	l_renderingServer->WaitCommandQueue(m_froxelizationRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);

	l_renderingServer->ExecuteCommandList(m_irraidanceInjectionRenderPassComp, GPUEngineType::Graphics);
	l_renderingServer->WaitCommandQueue(m_irraidanceInjectionRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);
	l_renderingServer->ExecuteCommandList(m_irraidanceInjectionRenderPassComp, GPUEngineType::Compute);
	l_renderingServer->WaitCommandQueue(m_irraidanceInjectionRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Compute);

	l_renderingServer->ExecuteCommandList(m_rayMarchingRenderPassComp, GPUEngineType::Graphics);
	l_renderingServer->WaitCommandQueue(m_rayMarchingRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);
	l_renderingServer->ExecuteCommandList(m_rayMarchingRenderPassComp, GPUEngineType::Compute);
	l_renderingServer->WaitCommandQueue(m_rayMarchingRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Compute);

	if (visualize)
	{
		l_renderingServer->ExecuteCommandList(m_visualizationRenderPassComp, GPUEngineType::Graphics);
		l_renderingServer->WaitCommandQueue(m_visualizationRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	}

	return true;
}

bool VolumetricPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->DeleteRenderPassComponent(m_froxelizationRenderPassComp);
	l_renderingServer->DeleteRenderPassComponent(m_irraidanceInjectionRenderPassComp);
	l_renderingServer->DeleteRenderPassComponent(m_rayMarchingRenderPassComp);
	l_renderingServer->DeleteRenderPassComponent(m_visualizationRenderPassComp);

	return true;
}

GPUResourceComponent *VolumetricPass::GetRayMarchingResult()
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

GPUResourceComponent *VolumetricPass::GetVisualizationResult()
{
	return m_visualizationRenderPassComp->m_RenderTargets[0].m_Texture;
}