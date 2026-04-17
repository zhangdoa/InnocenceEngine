#include "VolumetricPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/PerFrameDataService.h"
#include "../../Engine/Services/LightDataService.h"
#include "../../Engine/Services/DrawCallService.h"
#include "../../Engine/Services/GraphicsHardwareService.h"
#include "../../Engine/Services/FrameManagementService.h"

#include "OpaquePass.h"
#include "PreTAAPass.h"
#include "SunShadowGeometryProcessPass.h"
#include "LightCullingPass.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/TextureResourceService.h"
#include "../../Engine/Services/SamplerResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/CommandListResourceService.h"
using namespace Inno;

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

	CommandListComponent *m_froxelizationCommandListComp;
	CommandListComponent *m_irraidanceInjectionCommandListComp;
	CommandListComponent *m_rayMarchingCommandListComp;
	CommandListComponent *m_visualizationCommandListComp;

	TVec4<uint32_t> m_voxelizationResolution = TVec4<uint32_t>(160, 90, 64, 0);
	static bool m_isPassA = true;
} // namespace VolumetricPass

bool VolumetricPass::setupGeometryProcessPass()
{

	m_froxelizationSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricGeometryProcessPass/");

	m_froxelizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricGeometryProcessPass.vert/";
	m_froxelizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricGeometryProcessPass.geom/";
	m_froxelizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricGeometryProcessPass.frag/";

	m_froxelizationRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricGeometryProcessPass/");

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

	m_irraidanceInjectionSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricIrraidanceInjectionPass/");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricIrraidanceInjectionPass.comp/";

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_irraidanceInjectionRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricIrraidanceInjectionPass/");

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

	m_rayMarchingSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricRayMarchingPass/");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricRayMarchingPass.comp/";

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_rayMarchingRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricRayMarchingPass/");

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

	m_visualizationSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricVisualizationPass/");

	m_visualizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricVisualizationPass.vert/";
	m_visualizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricVisualizationPass.frag/";

	m_visualizationRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricVisualizationPass/");

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

	m_SamplerComp = g_Engine->Get<SamplerResourceService>()->Add("VolumetricPass/");

	setupGeometryProcessPass();
	setupIrradianceInjectionPass();
	setupRayMarchingPass();
	setupVisualizationPass();

	// Add command list components for each render pass
	m_froxelizationCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/Froxelization/Graphics/");
	m_froxelizationCommandListComp->m_Type = GPUEngineType::Graphics;

	m_irraidanceInjectionCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/IrraidanceInjection/Compute/");
	m_irraidanceInjectionCommandListComp->m_Type = GPUEngineType::Compute;

	m_rayMarchingCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/RayMarching/Compute/");
	m_rayMarchingCommandListComp->m_Type = GPUEngineType::Compute;

	m_visualizationCommandListComp = g_Engine->Get<CommandListResourceService>()->Add("VolumetricPass/Visualization/Graphics/");
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

	m_irraidanceInjectionResult = g_Engine->Get<TextureResourceService>()->Add("VolumetricIrraidanceInjectionResult/");
	m_irraidanceInjectionResult->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_A = g_Engine->Get<TextureResourceService>()->Add("VolumetricRayMarchingResult_A/");
	m_rayMarchingResult_A->m_TextureDesc = l_textureDesc;

	m_rayMarchingResult_B = g_Engine->Get<TextureResourceService>()->Add("VolumetricRayMarchingResult_B/");
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

bool VolumetricPass::froxelization()
{

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_MeshGPUBufferComp = g_Engine->Get<DrawCallService>()->GetGPUModelDataBuffer();
	auto l_MaterialGPUBufferComp = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();

	// l_fmService->CommandListBegin(m_froxelizationCommandListComp, m_froxelizationRenderPassComp, 0);
	// l_fmService->BindRenderPassComponent(m_froxelizationRenderPassComp, m_froxelizationCommandListComp);
	// l_fmService->ClearRenderTargets(m_froxelizationRenderPassComp, m_froxelizationCommandListComp);

	// l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[0], 3);
	// l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[1], 4);

	// for (uint32_t i = 0; i < l_drawCallCount; i++)
	// {
	// 	auto l_drawCallData = l_drawCallInfo[i];
	// 	auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::MainCamera);
	// 	if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
	// 	{
	// 		if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
	// 		{
	// 			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
	// 			{
	// 				l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.m_PerObjectConstantBufferIndex, 1);
	// 				l_fmService->BindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.m_PerObjectConstantBufferIndex, 1);

	// 				l_fmService->DrawIndexedInstanced(m_froxelizationRenderPassComp, l_drawCallData.mesh);
	// 			}
	// 		}
	// 	}
	// }

	// l_fmService->UnbindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[0], 3);
	// l_fmService->UnbindGPUResource(m_froxelizationRenderPassComp, ShaderStage::Pixel, m_froxelizationRenderPassComp->m_RenderTargets[1], 4);

	// l_fmService->CommandListEnd(m_froxelizationRenderPassComp, m_froxelizationCommandListComp);

	return true;
}

bool VolumetricPass::irraidanceInjection()
{

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_PointLightGPUBufferComp = g_Engine->Get<LightDataService>()->GetPointLightBuffer();
	auto l_CSMGPUBufferComp = g_Engine->Get<LightDataService>()->GetCSMBuffer();
	// TODO: Implement per-pass dispatch params buffer for VolumetricPass

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = m_voxelizationResolution.z;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = (uint32_t)std::ceil((float)l_numThreadsZ / 8.0f);

	DispatchParamsConstantBuffer l_irraidanceInjectionWorkload;
	l_irraidanceInjectionWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_irraidanceInjectionWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// TODO: Implement per-pass dispatch params buffer upload
	// g_Engine->Get<GPUBufferResourceService>()->Upload(l_dispatchParamsGPUBufferComp, &l_irraidanceInjectionWorkload, 6, 1);

	// l_fmService->CommandListBegin(m_irraidanceInjectionCommandListComp, m_irraidanceInjectionRenderPassComp, 0);
	// l_fmService->BindRenderPassComponent(m_irraidanceInjectionRenderPassComp, m_irraidanceInjectionCommandListComp);
	// l_fmService->ClearRenderTargets(m_irraidanceInjectionRenderPassComp, m_irraidanceInjectionCommandListComp);

	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_SamplerComp, 9);

	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_PointLightGPUBufferComp, 1);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_CSMGPUBufferComp, 2);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 3);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 4);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0], 5);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 6);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 7);
	// l_fmService->BindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 8);

	// l_fmService->Dispatch(m_irraidanceInjectionRenderPassComp, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	// l_fmService->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 4);
	// l_fmService->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0], 5);
	// l_fmService->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, SunShadowGeometryProcessPass::Get().GetResult(), 6);
	// l_fmService->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightGrid(), 7);
	// l_fmService->UnbindGPUResource(m_irraidanceInjectionRenderPassComp, ShaderStage::Compute, LightCullingPass::Get().GetLightIndexList(), 8);

	// l_fmService->CommandListEnd(m_irraidanceInjectionRenderPassComp, m_irraidanceInjectionCommandListComp);

	return true;
}

bool VolumetricPass::rayMarching()
{	
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

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	// TODO: Implement per-pass dispatch params buffer for VolumetricPass

	auto l_numThreadsX = m_voxelizationResolution.x;
	auto l_numThreadsY = m_voxelizationResolution.y;
	auto l_numThreadsZ = 1;
	auto l_numThreadGroupsX = (uint32_t)std::ceil((float)l_numThreadsX / 8.0f);
	auto l_numThreadGroupsY = (uint32_t)std::ceil((float)l_numThreadsY / 8.0f);
	auto l_numThreadGroupsZ = 1;

	DispatchParamsConstantBuffer l_rayMarchingWorkload;
	l_rayMarchingWorkload.numThreadGroups = TVec4<uint32_t>(l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ, 0);
	l_rayMarchingWorkload.numThreads = TVec4<uint32_t>(l_numThreadsX, l_numThreadsY, l_numThreadsZ, 0);

	// TODO: Implement per-pass dispatch params buffer upload
	// g_Engine->Get<GPUBufferResourceService>()->Upload(l_dispatchParamsGPUBufferComp, &l_rayMarchingWorkload, 7, 1);

	// l_fmService->CommandListBegin(m_rayMarchingCommandListComp, m_rayMarchingRenderPassComp, 0);
	// l_fmService->BindRenderPassComponent(m_rayMarchingRenderPassComp, m_rayMarchingCommandListComp);
	// l_fmService->ClearRenderTargets(m_rayMarchingRenderPassComp, m_rayMarchingCommandListComp);

	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_SamplerComp, 7);

	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_dispatchParamsGPUBufferComp, 1);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 2);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0], 3);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[1], 4);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_historyResultBinder, 5);
	// l_fmService->BindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_currentResultBinder, 6);

	// l_fmService->Dispatch(m_rayMarchingRenderPassComp, l_numThreadGroupsX, l_numThreadGroupsY, l_numThreadGroupsZ);

	// l_fmService->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_irraidanceInjectionResult, 2);
	// l_fmService->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[0], 3);
	// l_fmService->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, m_froxelizationRenderPassComp->m_RenderTargets[1], 4);
	// l_fmService->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_historyResultBinder, 5);
	// l_fmService->UnbindGPUResource(m_rayMarchingRenderPassComp, ShaderStage::Compute, l_currentResultBinder, 6);

	// l_fmService->CommandListEnd(m_rayMarchingRenderPassComp, m_rayMarchingCommandListComp);

	return true;
}

bool VolumetricPass::visualization(GPUResourceComponent *input)
{
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	auto l_PerFrameCBufferGPUBufferComp = g_Engine->Get<PerFrameDataService>()->GetCurrentFrameBuffer();
	auto l_MeshGPUBufferComp = g_Engine->Get<DrawCallService>()->GetGPUModelDataBuffer();
	auto l_MaterialGPUBufferComp = g_Engine->Get<DrawCallService>()->GetMaterialBuffer();

	l_fmService->CommandListBegin(m_visualizationRenderPassComp, m_visualizationCommandListComp, 0);
	l_fmService->BindRenderPassComponent(m_visualizationRenderPassComp, m_visualizationCommandListComp);
	l_fmService->ClearRenderTargets(m_visualizationRenderPassComp, m_visualizationCommandListComp);

	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, m_SamplerComp, 4);

	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Vertex, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, l_PerFrameCBufferGPUBufferComp, 0);
	// l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, input, 3);

	// for (uint32_t i = 0; i < l_drawCallCount; i++)
	// {
	// 	auto l_drawCallData = l_drawCallInfo[i];
	// 	auto l_visible = static_cast<uint32_t>(l_drawCallData.m_VisibilityMask & VisibilityMask::MainCamera);
	// 	if (l_visible && l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
	// 	{
	// 		if (l_drawCallData.material->m_ShaderModel == ShaderModel::Volumetric)
	// 		{
	// 			if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
	// 			{
	// 				l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Vertex, l_MeshGPUBufferComp, 1, l_drawCallData.m_PerObjectConstantBufferIndex, 1);
	// 				l_fmService->BindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, l_MaterialGPUBufferComp, 2, l_drawCallData.m_PerObjectConstantBufferIndex, 1);

	// 				l_fmService->DrawIndexedInstanced(m_visualizationRenderPassComp, l_drawCallData.mesh);
	// 			}
	// 		}
	// 	}
	// }

	// l_fmService->UnbindGPUResource(m_visualizationRenderPassComp, ShaderStage::Pixel, input, 3);

	// l_fmService->CommandListEnd(m_visualizationRenderPassComp, m_visualizationCommandListComp);

	return true;
}

bool VolumetricPass::ExecuteCommands(bool visualize)
{
	auto l_hwService = g_Engine->Get<GraphicsHardwareService>();
	auto l_fmService = g_Engine->Get<FrameManagementService>();

	froxelization();
	irraidanceInjection();
	rayMarching();

	if (visualize)
	{
		visualization(m_rayMarchingResult_A);
	}

	// Note: Command lists are prepared in the functions above but currently commented out
	// TODO: Implement proper command list preparation and execution
	// auto l_cmdList1 = froxelizationCommandList();
	// if (l_cmdList1) { l_graphicsService->Execute(l_cmdList1, GPUEngineType::Graphics); }
	l_hwService->WaitOnGPU(m_froxelizationRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	l_hwService->WaitOnGPU(m_froxelizationRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);

	// auto l_cmdList2 = irraidanceInjectionCommandList();
	// if (l_cmdList2) { l_graphicsService->Execute(l_cmdList2, GPUEngineType::Graphics); }
	l_hwService->WaitOnGPU(m_irraidanceInjectionRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);
	// if (l_cmdList2) { l_graphicsService->Execute(l_cmdList2, GPUEngineType::Compute); }
	l_hwService->WaitOnGPU(m_irraidanceInjectionRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Compute);

	// auto l_cmdList3 = rayMarchingCommandList();
	// if (l_cmdList3) { l_graphicsService->Execute(l_cmdList3, GPUEngineType::Graphics); }
	l_hwService->WaitOnGPU(m_rayMarchingRenderPassComp, GPUEngineType::Compute, GPUEngineType::Graphics);
	// if (l_cmdList3) { l_graphicsService->Execute(l_cmdList3, GPUEngineType::Compute); }
	l_hwService->WaitOnGPU(m_rayMarchingRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Compute);

	if (visualize)
	{
		// auto l_cmdList4 = visualizationCommandList();
		// if (l_cmdList4) { l_graphicsService->Execute(l_cmdList4, GPUEngineType::Graphics); }
		l_hwService->WaitOnGPU(m_visualizationRenderPassComp, GPUEngineType::Graphics, GPUEngineType::Graphics);
	}

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
	//return m_visualizationRenderPassComp->m_RenderTargets[0].m_Texture;
	return false;
}