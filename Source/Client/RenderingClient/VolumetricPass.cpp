#include "VolumetricPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"
#include "SunShadowBlurEvenPass.h"
#include "LightCullingPass.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

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

	SamplerDataComponent *m_SDC;

	RenderPassDataComponent *m_froxelizationRPDC;
	ShaderProgramComponent *m_froxelizationSPC;

	RenderPassDataComponent *m_visualizationRPDC;
	ShaderProgramComponent *m_visualizationSPC;

	RenderPassDataComponent *m_irraidanceInjectionRPDC;
	ShaderProgramComponent *m_irraidanceInjectionSPC;

	RenderPassDataComponent *m_rayMarchingRPDC;
	ShaderProgramComponent *m_rayMarchingSPC;

	TextureDataComponent *m_irraidanceInjectionResult;
	TextureDataComponent *m_rayMarchingResult_A;
	TextureDataComponent *m_rayMarchingResult_B;

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

	m_froxelizationRPDC = l_renderingServer->AddRenderPassDataComponent("VolumetricGeometryProcessPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 2;
	l_RenderPassDesc.m_IsOffScreen = true;
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

	m_froxelizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_froxelizationRPDC->m_ResourceBindingLayoutDescs.resize(5);
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 1;
	m_froxelizationRPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_froxelizationRPDC->m_ShaderProgram = m_froxelizationSPC;

	return true;
}

bool VolumetricPass::setupIrradianceInjectionPass()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	m_irraidanceInjectionSPC = l_renderingServer->AddShaderProgramComponent("VolumetricIrraidanceInjectionPass/");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricIrraidanceInjectionPass.comp/";

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_irraidanceInjectionRPDC = l_renderingServer->AddRenderPassDataComponent("VolumetricIrraidanceInjectionPass/");

	m_irraidanceInjectionRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs.resize(10);
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 3;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 5;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 6;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Image;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[7].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[7].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 2;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 1;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[8].m_GPUResourceType = GPUResourceType::Buffer;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[8].m_BindingAccessibility = Accessibility::ReadOnly;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[8].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[8].m_DescriptorSetIndex = 2;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[8].m_DescriptorIndex = 2;

	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[9].m_GPUResourceType = GPUResourceType::Sampler;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[9].m_DescriptorSetIndex = 3;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[9].m_DescriptorIndex = 0;
	m_irraidanceInjectionRPDC->m_ResourceBindingLayoutDescs[9].m_IndirectBinding = true;

	m_irraidanceInjectionRPDC->m_ShaderProgram = m_irraidanceInjectionSPC;

	return true;
}

bool VolumetricPass::setupRayMarchingPass()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	m_rayMarchingSPC = l_renderingServer->AddShaderProgramComponent("VolumetricRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricRayMarchingPass.comp/";

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_rayMarchingRPDC = l_renderingServer->AddRenderPassDataComponent("VolumetricRayMarchingPass/");

	m_rayMarchingRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs.resize(8);
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 6;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[2].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 1;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[4].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 1;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 2;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[5].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[5].m_BindingAccessibility = Accessibility::ReadOnly;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorSetIndex = 1;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[5].m_DescriptorIndex = 3;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[5].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[6].m_GPUResourceType = GPUResourceType::Image;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[6].m_BindingAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[6].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[6].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[7].m_GPUResourceType = GPUResourceType::Sampler;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorSetIndex = 3;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[7].m_DescriptorIndex = 0;
	m_rayMarchingRPDC->m_ResourceBindingLayoutDescs[7].m_IndirectBinding = true;

	m_rayMarchingRPDC->m_ShaderProgram = m_rayMarchingSPC;

	return true;
}

bool VolumetricPass::setupVisualizationPass()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	m_visualizationSPC = l_renderingServer->AddShaderProgramComponent("VolumetricVisualizationPass/");

	m_visualizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricVisualizationPass.vert/";
	m_visualizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricVisualizationPass.frag/";

	m_visualizationRPDC = l_renderingServer->AddRenderPassDataComponent("VolumetricVisualizationPass/");

	auto l_RenderPassDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc();
	auto l_viewportSize = g_Engine->getRenderingFrontend()->getScreenResolution();

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

	m_visualizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_visualizationRPDC->m_ResourceBindingLayoutDescs.resize(5);
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;

	m_visualizationRPDC->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 0;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 1;

	m_visualizationRPDC->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 0;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 2;

	m_visualizationRPDC->m_ResourceBindingLayoutDescs[3].m_GPUResourceType = GPUResourceType::Image;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[3].m_BindingAccessibility = Accessibility::ReadOnly;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[3].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorSetIndex = 1;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[3].m_DescriptorIndex = 0;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[3].m_IndirectBinding = true;

	m_visualizationRPDC->m_ResourceBindingLayoutDescs[4].m_GPUResourceType = GPUResourceType::Sampler;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorSetIndex = 2;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[4].m_DescriptorIndex = 0;
	m_visualizationRPDC->m_ResourceBindingLayoutDescs[4].m_IndirectBinding = true;

	m_visualizationRPDC->m_ShaderProgram = m_visualizationSPC;

	return true;
}

bool VolumetricPass::Setup()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	m_SDC = l_renderingServer->AddSamplerDataComponent("VolumetricPass/");

	setupGeometryProcessPass();
	setupIrradianceInjectionPass();
	setupRayMarchingPass();
	setupVisualizationPass();

	////
	auto l_textureDesc = g_Engine->getRenderingFrontend()->getDefaultRenderPassDesc().m_RenderTargetDesc;

	l_textureDesc.Sampler = TextureSampler::Sampler3D;
	l_textureDesc.Usage = TextureUsage::Sample;
	l_textureDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_textureDesc.Width = m_voxelizationResolution.x;
	l_textureDesc.Height = m_voxelizationResolution.y;
	l_textureDesc.DepthOrArraySize = m_voxelizationResolution.z;
	l_textureDesc.PixelDataType = TexturePixelDataType::Float32;

	m_irraidanceInjectionResult = l_renderingServer->AddTextureDataComponent("VolumetricIrraidanceInjectionResult/");
	m_irraidanceInjectionResult->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_A = l_renderingServer->AddTextureDataComponent("VolumetricRayMarchingResult_A/");
	m_rayMarchingResult_A->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_B = l_renderingServer->AddTextureDataComponent("VolumetricRayMarchingResult_B/");
	m_rayMarchingResult_B->m_TextureDesc = l_textureDesc;

	return true;
}

bool VolumetricPass::Initialize()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	l_renderingServer->InitializeSamplerDataComponent(m_SDC);

	l_renderingServer->InitializeShaderProgramComponent(m_froxelizationSPC);
	l_renderingServer->InitializeRenderPassDataComponent(m_froxelizationRPDC);

	l_renderingServer->InitializeShaderProgramComponent(m_irraidanceInjectionSPC);
	l_renderingServer->InitializeRenderPassDataComponent(m_irraidanceInjectionRPDC);

	l_renderingServer->InitializeShaderProgramComponent(m_rayMarchingSPC);
	l_renderingServer->InitializeRenderPassDataComponent(m_rayMarchingRPDC);

	l_renderingServer->InitializeShaderProgramComponent(m_visualizationSPC);
	l_renderingServer->InitializeRenderPassDataComponent(m_visualizationRPDC);

	l_renderingServer->InitializeTextureDataComponent(m_irraidanceInjectionResult);
	l_renderingServer->InitializeTextureDataComponent(m_rayMarchingResult_A);
	l_renderingServer->InitializeTextureDataComponent(m_rayMarchingResult_B);

	return true;
}

bool VolumetricPass::froxelization()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	l_renderingServer->CommandListBegin(m_froxelizationRPDC, 0);
	l_renderingServer->BindRenderPassDataComponent(m_froxelizationRPDC);
	l_renderingServer->CleanRenderTargets(m_froxelizationRPDC);

	l_renderingServer->BindGPUResource(m_froxelizationRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargets[0], 3, Accessibility::ReadWrite);
	l_renderingServer->BindGPUResource(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargets[1], 4, Accessibility::ReadWrite);

	auto &l_drawCallInfo = g_Engine->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					l_renderingServer->BindGPUResource(m_froxelizationRPDC, ShaderStage::Vertex, l_MeshGBDC, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					l_renderingServer->BindGPUResource(m_froxelizationRPDC, ShaderStage::Pixel, l_MaterialGBDC, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					l_renderingServer->DrawIndexedInstanced(m_froxelizationRPDC, l_drawCallData.mesh);
				}
			}
		}
	}

	l_renderingServer->UnbindGPUResource(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargets[0], 3, Accessibility::ReadWrite);
	l_renderingServer->UnbindGPUResource(m_froxelizationRPDC, ShaderStage::Pixel, m_froxelizationRPDC->m_RenderTargets[1], 4, Accessibility::ReadWrite);

	l_renderingServer->CommandListEnd(m_froxelizationRPDC);

	return true;
}

bool VolumetricPass::irraidanceInjection()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_PointLightGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PointLight);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = m_voxelizationResolution.z;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = (uint32_t)std::ceil((float)l_numThreadsZ / 8.0f);

	DispatchParamsConstantBuffer l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	l_renderingServer->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_irraidanceInjectionWorkload, 6, 1);

	l_renderingServer->CommandListBegin(m_irraidanceInjectionRPDC, 0);
	l_renderingServer->BindRenderPassDataComponent(m_irraidanceInjectionRPDC);
	l_renderingServer->CleanRenderTargets(m_irraidanceInjectionRPDC);

	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_SDC, 9);

	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_PointLightGBDC, 1, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_CSMGBDC, 2, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, l_dispatchParamsGBDC, 3, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult, 4, Accessibility::ReadWrite);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargets[0], 5, Accessibility::ReadWrite);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 7, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 8, Accessibility::ReadOnly);

	l_renderingServer->Dispatch(m_irraidanceInjectionRPDC, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_irraidanceInjectionResult, 4, Accessibility::ReadWrite);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargets[0], 5, Accessibility::ReadWrite);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, SunShadowBlurEvenPass::Get().GetResult(), 6, Accessibility::ReadOnly);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 7, Accessibility::ReadOnly);
	l_renderingServer->UnbindGPUResource(m_irraidanceInjectionRPDC, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 8, Accessibility::ReadOnly);

	l_renderingServer->CommandListEnd(m_irraidanceInjectionRPDC);

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

	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_dispatchParamsGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::ComputeDispatchParam);

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = 1;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = 1;

	DispatchParamsConstantBuffer l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	l_renderingServer->UploadGPUBufferDataComponent(l_dispatchParamsGBDC, &l_rayMarchingWorkload, 7, 1);

	l_renderingServer->CommandListBegin(m_rayMarchingRPDC, 0);
	l_renderingServer->BindRenderPassDataComponent(m_rayMarchingRPDC);
	l_renderingServer->CleanRenderTargets(m_rayMarchingRPDC);

	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, m_SDC, 7, Accessibility::ReadOnly);

	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, l_dispatchParamsGBDC, 1, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult, 2, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargets[0], 3, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargets[1], 4, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, l_historyResultBinder, 5, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, l_currentResultBinder, 6, Accessibility::ReadWrite);

	l_renderingServer->Dispatch(m_rayMarchingRPDC, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	l_renderingServer->UnbindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, m_irraidanceInjectionResult, 2, Accessibility::ReadOnly);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargets[0], 3, Accessibility::ReadOnly);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, m_froxelizationRPDC->m_RenderTargets[1], 4, Accessibility::ReadOnly);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, l_historyResultBinder, 5, Accessibility::ReadOnly);
	l_renderingServer->UnbindGPUResource(m_rayMarchingRPDC, ShaderStage::Compute, l_currentResultBinder, 6, Accessibility::ReadWrite);

	l_renderingServer->CommandListEnd(m_rayMarchingRPDC);

	return true;
}

bool VolumetricPass::visualization(GPUResourceComponent *input)
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);

	l_renderingServer->CommandListBegin(m_visualizationRPDC, 0);
	l_renderingServer->BindRenderPassDataComponent(m_visualizationRPDC);
	l_renderingServer->CleanRenderTargets(m_visualizationRPDC);

	l_renderingServer->BindGPUResource(m_visualizationRPDC, ShaderStage::Pixel, m_SDC, 4);

	l_renderingServer->BindGPUResource(m_visualizationRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_visualizationRPDC, ShaderStage::Pixel, l_PerFrameCBufferGBDC, 0, Accessibility::ReadOnly);
	l_renderingServer->BindGPUResource(m_visualizationRPDC, ShaderStage::Pixel, input, 3, Accessibility::ReadOnly);

	auto &l_drawCallInfo = g_Engine->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					l_renderingServer->BindGPUResource(m_visualizationRPDC, ShaderStage::Vertex, l_MeshGBDC, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					l_renderingServer->BindGPUResource(m_visualizationRPDC, ShaderStage::Pixel, l_MaterialGBDC, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					l_renderingServer->DrawIndexedInstanced(m_visualizationRPDC, l_drawCallData.mesh);
				}
			}
		}
	}

	l_renderingServer->UnbindGPUResource(m_visualizationRPDC, ShaderStage::Pixel, input, 3, Accessibility::ReadOnly);

	l_renderingServer->CommandListEnd(m_visualizationRPDC);

	return true;
}

bool VolumetricPass::Render(bool visualize)
{
	froxelization();
	irraidanceInjection();
	rayMarching();

	if (visualize)
	{
		visualization(m_rayMarchingResult_A);
	}

	auto l_renderingServer = g_Engine->getRenderingServer();

	l_renderingServer->ExecuteCommandList(m_froxelizationRPDC, GPUEngineType::Graphics);
	l_renderingServer->WaitCommandQueue(m_froxelizationRPDC, GPUEngineType::Graphics, GPUEngineType::Graphics);

	l_renderingServer->ExecuteCommandList(m_irraidanceInjectionRPDC, GPUEngineType::Graphics);
	l_renderingServer->WaitCommandQueue(m_irraidanceInjectionRPDC, GPUEngineType::Graphics, GPUEngineType::Graphics);

	l_renderingServer->ExecuteCommandList(m_rayMarchingRPDC, GPUEngineType::Graphics);
	l_renderingServer->WaitCommandQueue(m_rayMarchingRPDC, GPUEngineType::Graphics, GPUEngineType::Graphics);

	if (visualize)
	{
		l_renderingServer->ExecuteCommandList(m_visualizationRPDC, GPUEngineType::Graphics);
		l_renderingServer->WaitCommandQueue(m_visualizationRPDC, GPUEngineType::Graphics, GPUEngineType::Graphics);
	}

	return true;
}

bool VolumetricPass::Terminate()
{
	auto l_renderingServer = g_Engine->getRenderingServer();
	l_renderingServer->DeleteRenderPassDataComponent(m_froxelizationRPDC);
	l_renderingServer->DeleteRenderPassDataComponent(m_irraidanceInjectionRPDC);
	l_renderingServer->DeleteRenderPassDataComponent(m_rayMarchingRPDC);
	l_renderingServer->DeleteRenderPassDataComponent(m_visualizationRPDC);

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
	return m_visualizationRPDC->m_RenderTargets[0];
}