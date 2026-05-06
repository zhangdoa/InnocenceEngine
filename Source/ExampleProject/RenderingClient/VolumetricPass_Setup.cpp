#include "VolumetricPass_Internal.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"

using namespace Inno;

bool VolumetricPass::setupGeometryProcessPass()
{

	m_froxelizationSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricGeometryProcessPass");

	m_froxelizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricGeometryProcessPass.vert";
	m_froxelizationSPC->m_ShaderFilePaths.m_GSPath = "volumetricGeometryProcessPass.geom";
	m_froxelizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricGeometryProcessPass.frag";

	m_froxelizationRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricGeometryProcessPass");

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

	m_irraidanceInjectionSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricIrraidanceInjectionPass");

	m_irraidanceInjectionSPC->m_ShaderFilePaths.m_CSPath = "volumetricIrraidanceInjectionPass.comp";

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_irraidanceInjectionRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricIrraidanceInjectionPass");

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

	m_rayMarchingSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricRayMarchingPass");

	m_rayMarchingSPC->m_ShaderFilePaths.m_CSPath = "volumetricRayMarchingPass.comp";

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();
	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_GPUEngineType = GPUEngineType::Compute;
	l_RenderPassDesc.m_Resizable = false;

	m_rayMarchingRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricRayMarchingPass");

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

	m_visualizationSPC = g_Engine->Get<ShaderProgramResourceService>()->Add("VolumetricVisualizationPass");

	m_visualizationSPC->m_ShaderFilePaths.m_VSPath = "volumetricVisualizationPass.vert";
	m_visualizationSPC->m_ShaderFilePaths.m_PSPath = "volumetricVisualizationPass.frag";

	m_visualizationRenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("VolumetricVisualizationPass");

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
