#include "VoxelizationPass.h"
#include "../DefaultGPUBuffers/DefaultGPUBuffers.h"
#include "SunShadowPass.h"
#include "OpaquePass.h"
#include "LightPass.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

using namespace DefaultGPUBuffers;

namespace VoxelizationPass
{
	bool setupGeometryProcessPass();
	bool setupConvertPass();
	bool setupMultiBouncePass();

	bool setupScreenSpaceFeedbackPass();
	bool setupRayTracingPass();

	bool setupVisualizationPass();

	bool geometryProcess();
	bool convert();
	bool multiBounce(TextureDataComponent* input, TextureDataComponent* output);

	bool screenSpaceFeedback(TextureDataComponent* output);
	bool rayTracing(TextureDataComponent* input, TextureDataComponent* output);

	bool visualization(TextureDataComponent* input);

	GPUBufferDataComponent* m_voxelizationPassCBufferGBDC;
	GPUBufferDataComponent* m_geometryProcessSBufferGBDC;
	GPUBufferDataComponent* m_rayTracingRaySBufferGBDC;
	GPUBufferDataComponent* m_rayTracingProbeIndexSBufferGBDC;

	RenderPassDataComponent* m_geometryProcessRPDC;
	ShaderProgramComponent* m_geometryProcessSPC;
	SamplerDataComponent* m_geometryProcessSDC;

	RenderPassDataComponent* m_convertRPDC;
	ShaderProgramComponent* m_convertSPC;

	RenderPassDataComponent* m_multiBounceRPDC;
	ShaderProgramComponent* m_multiBounceSPC;
	SamplerDataComponent* m_multiBounceSDC;

	RenderPassDataComponent* m_SSFeedBackRPDC;
	ShaderProgramComponent* m_SSFeedBackSPC;

	RenderPassDataComponent* m_rayTracingRPDC;
	ShaderProgramComponent* m_rayTracingSPC;
	SamplerDataComponent* m_rayTracingSDC;

	RenderPassDataComponent* m_visualizationRPDC;
	ShaderProgramComponent* m_visualizationSPC;

	TextureDataComponent* m_initialBounceVolume;
	TextureDataComponent* m_normalVolume;

	TextureDataComponent* m_multiBounceVolume;

	TextureDataComponent* m_SSFeedBackVolume;
	TextureDataComponent* m_rayTracingVolume;

	TextureDataComponent* m_resultVolume;

	uint32_t m_voxelizationResolution = 128;
	uint32_t m_rayTracingVolumeResolution = m_voxelizationResolution;

	uint32_t m_numCones = 16;
	uint32_t m_coneTracingStep = 2;
	float m_coneTracingMaxDistance = 128.0f;
	bool m_isInitialLoadScene = true;
	uint32_t m_maxRay = 8;
	uint32_t m_maxProbe = 8;

	std::function<void()> f_sceneLoadingFinishCallback;
	std::vector<Vec4> m_rayTracingRay;
	std::vector<TVec4<uint32_t>> m_rayTracingProbeIndex;
	std::default_random_engine m_generator;
	std::uniform_int_distribution<uint32_t> m_randomInt(0, m_rayTracingVolumeResolution);
}

bool VoxelizationPass::setupGeometryProcessPass()
{
	m_geometryProcessSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelGeometryProcessPass/");

	m_geometryProcessSPC->m_ShaderFilePaths.m_VSPath = "voxelGeometryProcessPass.vert/";
	m_geometryProcessSPC->m_ShaderFilePaths.m_GSPath = "voxelGeometryProcessPass.geom/";
	m_geometryProcessSPC->m_ShaderFilePaths.m_PSPath = "voxelGeometryProcessPass.frag/";

	m_geometryProcessRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelGeometryProcessPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_IsOffScreen = true;
	l_RenderPassDesc.m_UseOutputMerger = false;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler3D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::Sample;
	l_RenderPassDesc.m_RenderTargetDesc.GPUAccessibility = Accessibility::ReadWrite;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataFormat = TexturePixelDataFormat::R;
	l_RenderPassDesc.m_RenderTargetDesc.PixelDataType = TexturePixelDataType::UInt32;
	l_RenderPassDesc.m_RenderTargetDesc.Width = m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.Height = m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.DepthOrArraySize = m_voxelizationResolution;
	l_RenderPassDesc.m_RenderTargetDesc.UseMipMap = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)m_voxelizationResolution;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)m_voxelizationResolution;

	m_geometryProcessRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs.resize(13);
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 9;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_BinderAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 4;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 0;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 5;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceCount = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[5].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 6;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceCount = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[6].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[7].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorSetIndex = 7;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[7].m_DescriptorIndex = 2;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[7].m_ResourceCount = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[7].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[8].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorSetIndex = 8;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[8].m_DescriptorIndex = 3;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[8].m_ResourceCount = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[8].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[9].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorSetIndex = 9;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[9].m_DescriptorIndex = 4;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[9].m_ResourceCount = 1;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[9].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[10].m_ResourceBinderType = ResourceBinderType::Image;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorSetIndex = 10;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[10].m_DescriptorIndex = 5;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[10].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[11].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[11].m_DescriptorSetIndex = 11;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[11].m_DescriptorIndex = 0;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[11].m_IndirectBinding = true;

	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[12].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[12].m_DescriptorSetIndex = 12;
	m_geometryProcessRPDC->m_ResourceBinderLayoutDescs[12].m_DescriptorIndex = 5;

	m_geometryProcessRPDC->m_ShaderProgram = m_geometryProcessSPC;

	m_geometryProcessSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VoxelGeometryProcessPass/");

	m_geometryProcessSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_geometryProcessSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	return true;
}

bool VoxelizationPass::setupConvertPass()
{
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_initialBounceVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VoxelLuminanceVolume/");
	m_initialBounceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_initialBounceVolume->m_TextureDesc.Width = m_voxelizationResolution;
	m_initialBounceVolume->m_TextureDesc.Height = m_voxelizationResolution;
	m_initialBounceVolume->m_TextureDesc.DepthOrArraySize = m_voxelizationResolution;
	m_initialBounceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_initialBounceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_initialBounceVolume->m_TextureDesc.UseMipMap = true;

	m_normalVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VoxelNormalVolume/");
	m_normalVolume->m_TextureDesc = m_initialBounceVolume->m_TextureDesc;

	m_convertSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelConvertPass/");

	m_convertSPC->m_ShaderFilePaths.m_CSPath = "voxelConvertPass.comp/";

	m_convertRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelConvertPass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_convertRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_convertRPDC->m_ResourceBinderLayoutDescs.resize(4);

	m_convertRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_convertRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadWrite;
	m_convertRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_convertRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_convertRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;

	m_convertRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_convertRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadWrite;
	m_convertRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_convertRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 2;
	m_convertRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_convertRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_convertRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_convertRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_convertRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_convertRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 3;
	m_convertRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 2;
	m_convertRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_convertRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_convertRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 4;
	m_convertRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 9;

	m_convertRPDC->m_ShaderProgram = m_convertSPC;

	return true;
}

bool VoxelizationPass::setupMultiBouncePass()
{
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_multiBounceVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VoxelMultiBounceVolume/");
	m_multiBounceVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_multiBounceVolume->m_TextureDesc.Width = m_voxelizationResolution;
	m_multiBounceVolume->m_TextureDesc.Height = m_voxelizationResolution;
	m_multiBounceVolume->m_TextureDesc.DepthOrArraySize = m_voxelizationResolution;
	m_multiBounceVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_multiBounceVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_multiBounceVolume->m_TextureDesc.UseMipMap = true;

	m_multiBounceSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelMultiBouncePass/");

	m_multiBounceSPC->m_ShaderFilePaths.m_CSPath = "voxelMultiBouncePass.comp/";

	m_multiBounceRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelMultiBouncePass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_multiBounceRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_multiBounceRPDC->m_ResourceBinderLayoutDescs.resize(5);

	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_multiBounceRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 9;

	m_multiBounceRPDC->m_ShaderProgram = m_multiBounceSPC;

	m_multiBounceSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VoxelMultiBouncePass/");

	m_multiBounceSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_multiBounceSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;

	return true;
}

bool VoxelizationPass::setupScreenSpaceFeedbackPass()
{
	m_SSFeedBackSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelScreenSpaceFeedbackPass/");

	m_SSFeedBackSPC->m_ShaderFilePaths.m_CSPath = "voxelScreenSpaceFeedBackPass.comp/";

	m_SSFeedBackRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelScreenSpaceFeedbackPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_SSFeedBackVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VoxelScreenSpaceFeedbackVolume/");
	m_SSFeedBackVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_SSFeedBackVolume->m_TextureDesc.Width = m_rayTracingVolumeResolution;
	m_SSFeedBackVolume->m_TextureDesc.Height = m_rayTracingVolumeResolution;
	m_SSFeedBackVolume->m_TextureDesc.DepthOrArraySize = m_rayTracingVolumeResolution;
	m_SSFeedBackVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_SSFeedBackVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_SSFeedBackVolume->m_TextureDesc.UseMipMap = true;

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_SSFeedBackRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs.resize(5);

	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadOnly;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 0;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;

	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_SSFeedBackRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 9;

	m_SSFeedBackRPDC->m_ShaderProgram = m_SSFeedBackSPC;

	return true;
}

bool VoxelizationPass::setupRayTracingPass()
{
	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();

	m_rayTracingVolume = g_pModuleManager->getRenderingServer()->AddTextureDataComponent("VoxelRayTracingVolume/");
	m_rayTracingVolume->m_TextureDesc = l_RenderPassDesc.m_RenderTargetDesc;

	m_rayTracingVolume->m_TextureDesc.Width = m_rayTracingVolumeResolution;
	m_rayTracingVolume->m_TextureDesc.Height = m_rayTracingVolumeResolution;
	m_rayTracingVolume->m_TextureDesc.DepthOrArraySize = m_rayTracingVolumeResolution;
	m_rayTracingVolume->m_TextureDesc.Usage = TextureUsage::Sample;
	m_rayTracingVolume->m_TextureDesc.Sampler = TextureSampler::Sampler3D;
	m_rayTracingVolume->m_TextureDesc.UseMipMap = true;

	m_rayTracingSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelRayTracingPass/");

	m_rayTracingSPC->m_ShaderFilePaths.m_CSPath = "voxelRayTracingPass.comp/";

	m_rayTracingRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelRayTracingPass/");

	l_RenderPassDesc.m_RenderTargetCount = 0;
	l_RenderPassDesc.m_RenderPassUsage = RenderPassUsage::Compute;
	l_RenderPassDesc.m_IsOffScreen = true;

	m_rayTracingRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs.resize(7);

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 1;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[1].m_BinderAccessibility = Accessibility::ReadOnly;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 1;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[1].m_IndirectBinding = true;

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Image;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[2].m_BinderAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 0;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[2].m_IndirectBinding = true;

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[3].m_ResourceBinderType = ResourceBinderType::Sampler;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorSetIndex = 3;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[3].m_DescriptorIndex = 0;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[3].m_IndirectBinding = true;

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[4].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorSetIndex = 0;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[4].m_DescriptorIndex = 9;

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[5].m_BinderAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[5].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorSetIndex = 2;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[5].m_DescriptorIndex = 1;

	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[6].m_BinderAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[6].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorSetIndex = 2;
	m_rayTracingRPDC->m_ResourceBinderLayoutDescs[6].m_DescriptorIndex = 2;

	m_rayTracingRPDC->m_ShaderProgram = m_rayTracingSPC;

	m_rayTracingSDC = g_pModuleManager->getRenderingServer()->AddSamplerDataComponent("VoxelRayTracingPass/");

	m_rayTracingSDC->m_SamplerDesc.m_WrapMethodU = TextureWrapMethod::Repeat;
	m_rayTracingSDC->m_SamplerDesc.m_WrapMethodV = TextureWrapMethod::Repeat;
	m_rayTracingSDC->m_SamplerDesc.m_WrapMethodW = TextureWrapMethod::Repeat;

	return true;
}

bool VoxelizationPass::setupVisualizationPass()
{
	m_visualizationSPC = g_pModuleManager->getRenderingServer()->AddShaderProgramComponent("VoxelVisualizationPass/");

	m_visualizationSPC->m_ShaderFilePaths.m_VSPath = "voxelVisualizationPass.vert/";
	m_visualizationSPC->m_ShaderFilePaths.m_GSPath = "voxelVisualizationPass.geom/";
	m_visualizationSPC->m_ShaderFilePaths.m_PSPath = "voxelVisualizationPass.frag/";

	m_visualizationRPDC = g_pModuleManager->getRenderingServer()->AddRenderPassDataComponent("VoxelVisualizationPass/");

	auto l_RenderPassDesc = g_pModuleManager->getRenderingFrontend()->getDefaultRenderPassDesc();
	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_RenderTargetDesc.Sampler = TextureSampler::Sampler2D;
	l_RenderPassDesc.m_RenderTargetDesc.Usage = TextureUsage::ColorAttachment;
	l_RenderPassDesc.m_RenderTargetDesc.Width = l_viewportSize.x;
	l_RenderPassDesc.m_RenderTargetDesc.Height = l_viewportSize.y;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_PrimitiveTopology = PrimitiveTopology::Point;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Width = (float)l_viewportSize.x;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc.m_Height = (float)l_viewportSize.y;
	m_visualizationRPDC->m_RenderPassDesc = l_RenderPassDesc;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs.resize(3);

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceBinderType = ResourceBinderType::Image;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_BinderAccessibility = Accessibility::ReadOnly;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_ResourceAccessibility = Accessibility::ReadWrite;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_DescriptorIndex = 0;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[0].m_IndirectBinding = true;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[1].m_DescriptorIndex = 0;

	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_ResourceBinderType = ResourceBinderType::Buffer;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorSetIndex = 2;
	m_visualizationRPDC->m_ResourceBinderLayoutDescs[2].m_DescriptorIndex = 9;

	m_visualizationRPDC->m_ShaderProgram = m_visualizationSPC;

	return true;
}

bool VoxelizationPass::Setup()
{
	f_sceneLoadingFinishCallback = []()
	{
		m_isInitialLoadScene = true;
	};

	g_pModuleManager->getFileSystem()->addSceneLoadingFinishCallback(&f_sceneLoadingFinishCallback);

	////
	m_voxelizationPassCBufferGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VoxelizationPassCBuffer/");
	m_voxelizationPassCBufferGBDC->m_ElementCount = 1;
	m_voxelizationPassCBufferGBDC->m_ElementSize = sizeof(VoxelizationConstantBuffer);

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_voxelizationPassCBufferGBDC);

	////
	m_geometryProcessSBufferGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VoxelGeometryProcessSBuffer/");
	m_geometryProcessSBufferGBDC->m_ElementCount = m_voxelizationResolution * m_voxelizationResolution * m_voxelizationResolution * 2;
	m_geometryProcessSBufferGBDC->m_ElementSize = sizeof(uint32_t);
	m_geometryProcessSBufferGBDC->m_GPUAccessibility = Accessibility::ReadWrite;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_geometryProcessSBufferGBDC);

	////
	m_rayTracingRay.reserve(m_maxRay * m_maxRay);

	m_rayTracingProbeIndex.reserve(m_maxProbe * m_maxProbe * m_maxProbe);

	auto radius = 1.0f;
	auto sectorCount = m_maxRay;
	auto stackCount = m_maxRay;

	float x, y, z, xy;

	float sectorStep = 2 * PI<float> / sectorCount;
	float stackStep = PI<float> / stackCount;
	float sectorAngle, stackAngle;

	for (uint32_t i = 0; i < stackCount; ++i)
	{
		stackAngle = PI<float> / 2 - i * stackStep;
		xy = radius * cosf(stackAngle);
		z = radius * sinf(stackAngle);

		for (uint32_t j = 0; j < sectorCount; ++j)
		{
			sectorAngle = j * sectorStep;

			x = xy * cosf(sectorAngle);
			y = xy * sinf(sectorAngle);
			auto l_dir = Vec4(x, y, z, 0.0f);
			l_dir = l_dir.normalize();

			m_rayTracingRay.emplace_back(l_dir);
		}
	}

	m_rayTracingRaySBufferGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VoxelRayTracingRaySBuffer/");
	m_rayTracingRaySBufferGBDC->m_ElementCount = m_maxRay * m_maxRay;
	m_rayTracingRaySBufferGBDC->m_ElementSize = sizeof(Vec4);
	m_rayTracingRaySBufferGBDC->m_GPUAccessibility = Accessibility::ReadWrite;
	m_rayTracingRaySBufferGBDC->m_InitialData = &m_rayTracingRay[0];

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_rayTracingRaySBufferGBDC);

	////
	m_rayTracingProbeIndexSBufferGBDC = g_pModuleManager->getRenderingServer()->AddGPUBufferDataComponent("VoxelRayTracingProbeIndexSBuffer/");
	m_rayTracingProbeIndexSBufferGBDC->m_ElementCount = m_maxProbe * m_maxProbe * m_maxProbe;
	m_rayTracingProbeIndexSBufferGBDC->m_ElementSize = sizeof(TVec4<uint32_t>);
	m_rayTracingProbeIndexSBufferGBDC->m_GPUAccessibility = Accessibility::ReadWrite;

	g_pModuleManager->getRenderingServer()->InitializeGPUBufferDataComponent(m_rayTracingProbeIndexSBufferGBDC);

	setupGeometryProcessPass();
	setupConvertPass();
	setupMultiBouncePass();
	setupRayTracingPass();
	setupScreenSpaceFeedbackPass();
	setupVisualizationPass();

	return true;
}

bool VoxelizationPass::Initialize()
{
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_initialBounceVolume);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_normalVolume);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_multiBounceVolume);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_SSFeedBackVolume);
	g_pModuleManager->getRenderingServer()->InitializeTextureDataComponent(m_rayTracingVolume);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_geometryProcessSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_geometryProcessSDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_convertSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_convertRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_multiBounceSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_multiBounceRPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_multiBounceSDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_rayTracingSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_rayTracingRPDC);
	g_pModuleManager->getRenderingServer()->InitializeSamplerDataComponent(m_rayTracingSDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_SSFeedBackSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_SSFeedBackRPDC);

	g_pModuleManager->getRenderingServer()->InitializeShaderProgramComponent(m_visualizationSPC);
	g_pModuleManager->getRenderingServer()->InitializeRenderPassDataComponent(m_visualizationRPDC);

	return true;
}

bool VoxelizationPass::geometryProcess()
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);
	auto l_MeshGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Mesh);
	auto l_MaterialGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::Material);
	auto l_CSMGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::CSM);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_geometryProcessRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_geometryProcessRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessSDC->m_ResourceBinder, 11, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Vertex, l_PerFrameCBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Geometry, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 3, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 3, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessSBufferGBDC->m_ResourceBinder, 4, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, SunShadowPass::GetShadowMap(), 10, 5, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_CSMGBDC->m_ResourceBinder, 12, 5, Accessibility::ReadOnly);

	auto& l_drawCallInfo = g_pModuleManager->getRenderingFrontend()->getDrawCallInfo();
	auto l_drawCallCount = l_drawCallInfo.size();

	for (uint32_t i = 0; i < l_drawCallCount; i++)
	{
		auto l_drawCallData = l_drawCallInfo[i];
		if (l_drawCallData.material->m_ObjectStatus == ObjectStatus::Activated)
		{
			if (l_drawCallData.material->m_ShaderModel == ShaderModel::Opaque)
			{
				if (l_drawCallData.mesh->m_ObjectStatus == ObjectStatus::Activated)
				{
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Vertex, l_MeshGBDC->m_ResourceBinder, 1, 1, Accessibility::ReadOnly, l_drawCallData.meshConstantBufferIndex, 1);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_MaterialGBDC->m_ResourceBinder, 2, 2, Accessibility::ReadOnly, l_drawCallData.materialConstantBufferIndex, 1);

					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 5, 0);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 6, 1);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 7, 2);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 8, 3);
					g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 9, 4);

					g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_geometryProcessRPDC, l_drawCallData.mesh);

					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[0].m_Texture->m_ResourceBinder, 5, 0);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[1].m_Texture->m_ResourceBinder, 6, 1);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[2].m_Texture->m_ResourceBinder, 7, 2);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[3].m_Texture->m_ResourceBinder, 8, 3);
					g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, l_drawCallData.material->m_TextureSlots[4].m_Texture->m_ResourceBinder, 9, 4);
				}
			}
		}
	}

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, m_geometryProcessSBufferGBDC->m_ResourceBinder, 4, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_geometryProcessRPDC, ShaderStage::Pixel, SunShadowPass::GetShadowMap(), 10, 5, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_geometryProcessRPDC);

	return true;
}

bool VoxelizationPass::convert()
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_convertRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_convertRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_convertRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_convertRPDC, ShaderStage::Compute, m_geometryProcessSBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_convertRPDC, ShaderStage::Compute, m_initialBounceVolume->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_convertRPDC, ShaderStage::Compute, m_normalVolume->m_ResourceBinder, 2, 2, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_convertRPDC, ShaderStage::Compute, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 3, 9, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_convertRPDC, m_voxelizationResolution / 8, m_voxelizationResolution / 8, m_voxelizationResolution / 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_convertRPDC, ShaderStage::Compute, m_geometryProcessSBufferGBDC->m_ResourceBinder, 0, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_convertRPDC, ShaderStage::Compute, m_initialBounceVolume->m_ResourceBinder, 1, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_convertRPDC, ShaderStage::Compute, m_normalVolume->m_ResourceBinder, 2, 2, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_convertRPDC);

	g_pModuleManager->getRenderingServer()->GenerateMipmap(m_initialBounceVolume);
	g_pModuleManager->getRenderingServer()->GenerateMipmap(m_normalVolume);

	return true;
}

bool VoxelizationPass::multiBounce(TextureDataComponent* input, TextureDataComponent* output)
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_multiBounceRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_multiBounceRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_multiBounceRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 4, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, m_normalVolume->m_ResourceBinder, 1, 1, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, m_multiBounceSDC->m_ResourceBinder, 3, 0);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, input->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, output->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_multiBounceRPDC, m_voxelizationResolution / 8, m_voxelizationResolution / 8, m_voxelizationResolution / 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, input->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, output->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_multiBounceRPDC, ShaderStage::Compute, m_normalVolume->m_ResourceBinder, 1, 1, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_multiBounceRPDC);

	return true;
}

bool VoxelizationPass::screenSpaceFeedback(TextureDataComponent* output)
{
	auto l_viewportSize = g_pModuleManager->getRenderingFrontend()->getScreenResolution();
	auto l_perFrameGBDC = DefaultGPUBuffers::GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_SSFeedBackRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_SSFeedBackRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_SSFeedBackRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, l_perFrameGBDC->m_ResourceBinder, 3, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 4, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, LightPass::GetResult(1), 1, 1, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, output->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_SSFeedBackRPDC, uint32_t(l_viewportSize.x / 8.0f), uint32_t(l_viewportSize.y / 8.0f), 1);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, OpaquePass::GetRPDC()->m_RenderTargetsResourceBinders[0], 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, LightPass::GetResult(1), 1, 1, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_SSFeedBackRPDC, ShaderStage::Compute, output->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_SSFeedBackRPDC);

	return true;
}

bool VoxelizationPass::rayTracing(TextureDataComponent* input, TextureDataComponent* output)
{
	g_pModuleManager->getRenderingServer()->CommandListBegin(m_rayTracingRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_rayTracingRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_rayTracingRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 4, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, input->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_normalVolume->m_ResourceBinder, 1, 1, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, output->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_rayTracingSDC->m_ResourceBinder, 3, 0);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_rayTracingRaySBufferGBDC->m_ResourceBinder, 5, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_rayTracingProbeIndexSBufferGBDC->m_ResourceBinder, 6, 2, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->DispatchCompute(m_rayTracingRPDC, 8, 8, 8);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, input->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_normalVolume->m_ResourceBinder, 1, 1, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, output->m_ResourceBinder, 2, 0, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_rayTracingRaySBufferGBDC->m_ResourceBinder, 5, 1, Accessibility::ReadWrite);
	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_rayTracingRPDC, ShaderStage::Compute, m_rayTracingProbeIndexSBufferGBDC->m_ResourceBinder, 6, 2, Accessibility::ReadWrite);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_rayTracingRPDC);

	return true;
}

bool VoxelizationPass::visualization(TextureDataComponent* input)
{
	auto l_PerFrameCBufferGBDC = GetGPUBufferDataComponent(GPUBufferUsageType::PerFrame);

	g_pModuleManager->getRenderingServer()->CommandListBegin(m_visualizationRPDC, 0);
	g_pModuleManager->getRenderingServer()->BindRenderPassDataComponent(m_visualizationRPDC);
	g_pModuleManager->getRenderingServer()->CleanRenderTargets(m_visualizationRPDC);

	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, input->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Geometry, l_PerFrameCBufferGBDC->m_ResourceBinder, 1, 0, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 2, 9, Accessibility::ReadOnly);
	g_pModuleManager->getRenderingServer()->ActivateResourceBinder(m_visualizationRPDC, ShaderStage::Geometry, m_voxelizationPassCBufferGBDC->m_ResourceBinder, 2, 9, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->DispatchDrawCall(m_visualizationRPDC, m_voxelizationResolution * m_voxelizationResolution * m_voxelizationResolution);

	g_pModuleManager->getRenderingServer()->DeactivateResourceBinder(m_visualizationRPDC, ShaderStage::Vertex, input->m_ResourceBinder, 0, 0, Accessibility::ReadOnly);

	g_pModuleManager->getRenderingServer()->CommandListEnd(m_visualizationRPDC);

	return true;
}

bool VoxelizationPass::Render(bool visualize, uint32_t multiBounceCount, bool screenFeedback)
{
	if (screenFeedback)
	{
		auto l_cameraPos = g_pModuleManager->getRenderingFrontend()->getPerFrameConstantBuffer().camera_posWS;
		VoxelizationConstantBuffer l_voxelPassCB;

		l_voxelPassCB.volumeCenter = l_cameraPos;
		l_voxelPassCB.volumeExtend = 128.0f;
		l_voxelPassCB.volumeExtendRcp = 1.0f / l_voxelPassCB.volumeExtend;
		l_voxelPassCB.volumeResolution = (float)m_voxelizationResolution;
		l_voxelPassCB.volumeResolutionRcp = 1.0f / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSize = l_voxelPassCB.volumeExtend / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSizeRcp = 1.0f / l_voxelPassCB.voxelSize;
		l_voxelPassCB.numCones = (float)m_numCones;
		l_voxelPassCB.numConesRcp = 1.0f / l_voxelPassCB.numCones;
		l_voxelPassCB.coneTracingStep = (float)m_coneTracingStep;
		l_voxelPassCB.coneTracingMaxDistance = m_coneTracingMaxDistance;

		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_voxelizationPassCBufferGBDC, &l_voxelPassCB);

		auto l_tick = g_pModuleManager->getTimeSystem()->getCurrentTimeFromEpoch();
		m_generator.seed((uint32_t)l_tick);

		m_rayTracingProbeIndex.clear();

		for (uint32_t i = 0; i < m_maxProbe * m_maxProbe * m_maxProbe; i++)
		{
			auto l_sample = TVec4(m_randomInt(m_generator), m_randomInt(m_generator), m_randomInt(m_generator), (uint32_t)0);

			m_rayTracingProbeIndex.emplace_back(l_sample);
		}

		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_rayTracingProbeIndexSBufferGBDC, m_rayTracingProbeIndex);

		if (m_isInitialLoadScene)
		{
			m_isInitialLoadScene = false;

			geometryProcess();
			g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_geometryProcessRPDC);
			g_pModuleManager->getRenderingServer()->WaitForFrame(m_geometryProcessRPDC);

			convert();
			g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_convertRPDC);
			g_pModuleManager->getRenderingServer()->WaitForFrame(m_convertRPDC);

			g_pModuleManager->getRenderingServer()->CopyTextureDataComponent(m_initialBounceVolume, m_SSFeedBackVolume);
		}
		else
		{
			g_pModuleManager->getRenderingServer()->ClearTextureDataComponent(m_SSFeedBackVolume);

			screenSpaceFeedback(m_SSFeedBackVolume);
			g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_SSFeedBackRPDC);
			g_pModuleManager->getRenderingServer()->WaitForFrame(m_SSFeedBackRPDC);
		}

		rayTracing(m_SSFeedBackVolume, m_rayTracingVolume);
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_rayTracingRPDC);
		g_pModuleManager->getRenderingServer()->WaitForFrame(m_rayTracingRPDC);

		m_resultVolume = m_rayTracingVolume;
	}
	else
	{
		auto l_cameraPos = g_pModuleManager->getRenderingFrontend()->getPerFrameConstantBuffer().camera_posWS;
		auto l_sceneAABB = g_pModuleManager->getPhysicsSystem()->getStaticSceneAABB();
		auto l_maxExtend = std::max(std::max(l_sceneAABB.m_extend.x, l_sceneAABB.m_extend.y), l_sceneAABB.m_extend.z);
		auto l_adjustedBoundMax = l_sceneAABB.m_boundMin + Vec4(l_maxExtend, l_maxExtend, l_maxExtend, 0.0f);
		auto l_adjustedCenter = l_sceneAABB.m_boundMin + Vec4(l_maxExtend, l_maxExtend, l_maxExtend, 0.0f) / 2.0f;

		VoxelizationConstantBuffer l_voxelPassCB;

		l_voxelPassCB.volumeCenter = l_adjustedCenter;
		l_voxelPassCB.volumeExtend = l_maxExtend;
		l_voxelPassCB.volumeExtendRcp = 1.0f / l_voxelPassCB.volumeExtend;
		l_voxelPassCB.volumeResolution = (float)m_voxelizationResolution;
		l_voxelPassCB.volumeResolutionRcp = 1.0f / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSize = l_voxelPassCB.volumeExtend / l_voxelPassCB.volumeResolution;
		l_voxelPassCB.voxelSizeRcp = 1.0f / l_voxelPassCB.voxelSize;
		l_voxelPassCB.numCones = (float)m_numCones;
		l_voxelPassCB.numConesRcp = 1.0f / l_voxelPassCB.numCones;
		l_voxelPassCB.coneTracingStep = (float)m_coneTracingStep;
		l_voxelPassCB.coneTracingMaxDistance = m_coneTracingMaxDistance;

		g_pModuleManager->getRenderingServer()->UploadGPUBufferDataComponent(m_voxelizationPassCBufferGBDC, &l_voxelPassCB);

		////
		g_pModuleManager->getRenderingServer()->ClearGPUBufferDataComponent(m_geometryProcessSBufferGBDC);

		geometryProcess();
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_geometryProcessRPDC);
		g_pModuleManager->getRenderingServer()->WaitForFrame(m_geometryProcessRPDC);

		convert();
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_convertRPDC);
		g_pModuleManager->getRenderingServer()->WaitForFrame(m_convertRPDC);

		if (multiBounceCount)
		{
			auto l_input = m_initialBounceVolume;
			auto l_output = m_multiBounceVolume;

			for (uint32_t i = 0; i < multiBounceCount; i++)
			{
				multiBounce(l_input, l_output);

				g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_multiBounceRPDC);
				g_pModuleManager->getRenderingServer()->WaitForFrame(m_multiBounceRPDC);

				g_pModuleManager->getRenderingServer()->GenerateMipmap(l_output);

				m_resultVolume = l_output;

				auto l_temp = l_input;
				l_input = l_output;
				l_output = l_temp;
			}
		}
		else
		{
			m_resultVolume = m_initialBounceVolume;
		}
	}

	if (visualize)
	{
		visualization(m_resultVolume);
		g_pModuleManager->getRenderingServer()->ExecuteCommandList(m_visualizationRPDC);
		g_pModuleManager->getRenderingServer()->WaitForFrame(m_visualizationRPDC);
	}

	return true;
}

bool VoxelizationPass::Terminate()
{
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_geometryProcessRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_convertRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_multiBounceRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_SSFeedBackRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_rayTracingRPDC);
	g_pModuleManager->getRenderingServer()->DeleteRenderPassDataComponent(m_visualizationRPDC);

	return true;
}

IResourceBinder* VoxelizationPass::GetVoxelizationLuminanceVolume()
{
	return m_resultVolume->m_ResourceBinder;
}

IResourceBinder* VoxelizationPass::GetVisualizationResult()
{
	return m_visualizationRPDC->m_RenderTargets[0]->m_ResourceBinder;
}

IResourceBinder* VoxelizationPass::GetVoxelizationCBuffer()
{
	return m_voxelizationPassCBufferGBDC->m_ResourceBinder;
}