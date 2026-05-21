#include "DebugPass.h"

#include "../../Engine/Services/RenderingConfigurationService.h"
#include "../../Engine/Services/TemplateAssetService.h"
#include "../../Engine/Services/BVHService.h"

#include "../../Engine/Engine.h"
#include "../../Engine/Services/ShaderProgramResourceService.h"
#include "../../Engine/Services/RenderPassResourceService.h"
#include "../../Engine/Services/GPUBufferResourceService.h"
#include "../../Engine/Services/MeshResourceService.h"
using namespace Inno;

bool DebugPass::Setup(IServiceConfig *systemConfig)
{
	
	auto l_cameraFrustumMeshCount = 4;
	m_debugCameraFrustumMeshComps.resize(l_cameraFrustumMeshCount);
	for (size_t i = 0; i < l_cameraFrustumMeshCount; i++)
	{
		m_debugCameraFrustumMeshComps[i] = g_Engine->Get<MeshResourceService>()->Add(("DebugCameraFrustumMesh_" + std::to_string(i)).c_str());
		g_Engine->Get<TemplateAssetService>()->GenerateMesh(MeshShape::Cube, m_debugCameraFrustumMeshComps[i]);
		}
	
	m_debugSphereMeshGPUBufferComp = g_Engine->Get<GPUBufferResourceService>()->Add("DebugSphereMeshGPUBuffer");
	m_debugSphereMeshGPUBufferComp->m_ElementCount = m_maxDebugMeshes;
	m_debugSphereMeshGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugSphereMeshGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugCubeMeshGPUBufferComp = g_Engine->Get<GPUBufferResourceService>()->Add("DebugCubeMeshGPUBuffer");
	m_debugCubeMeshGPUBufferComp->m_ElementCount = m_maxDebugMeshes;
	m_debugCubeMeshGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugCubeMeshGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugCameraFrustumGPUBufferComp = g_Engine->Get<GPUBufferResourceService>()->Add("DebugCameraFrustumGPUBuffer");
	m_debugCameraFrustumGPUBufferComp->m_ElementCount = l_cameraFrustumMeshCount;
	m_debugCameraFrustumGPUBufferComp->m_ElementSize = sizeof(DebugPerObjectConstantBuffer);
	m_debugCameraFrustumGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	m_debugMaterialGPUBufferComp = g_Engine->Get<GPUBufferResourceService>()->Add("DebugMaterialGPUBuffer");
	m_debugMaterialGPUBufferComp->m_ElementCount = m_maxDebugMaterial;
	m_debugMaterialGPUBufferComp->m_ElementSize = sizeof(DebugMaterialConstantBuffer);
	m_debugMaterialGPUBufferComp->m_GPUAccessibility = Accessibility::ReadWrite;

	////
	m_ShaderProgramComp = g_Engine->Get<ShaderProgramResourceService>()->Add("DebugPass");

	m_ShaderProgramComp->m_ShaderFilePaths.m_VSPath = "debugPass.vert";
	m_ShaderProgramComp->m_ShaderFilePaths.m_PSPath = "debugPass.frag";
	m_RenderPassComp = g_Engine->Get<RenderPassResourceService>()->Add("DebugPass");

	auto l_RenderPassDesc = g_Engine->Get<RenderingConfigurationService>()->GetDefaultRenderPassDesc();

	l_RenderPassDesc.m_RenderTargetCount = 1;
	l_RenderPassDesc.m_UseDepthBuffer = true;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_AllowDepthWrite = true;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthComparisionFunction = ComparisionFunction::LessEqual;

	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_UseCulling = false;
	l_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc.m_RasterizerFillMode = RasterizerFillMode::Wireframe;

	m_RenderPassComp->m_RenderPassDesc = l_RenderPassDesc;

	m_RenderPassComp->m_ResourceBindingLayoutDescs.resize(3);
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorSetIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_DescriptorIndex = 0;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[0].m_ShaderStage = ShaderStage::Vertex;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_DescriptorIndex = 0;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[1].m_ResourceAccessibility = Accessibility::ReadWrite;

	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_GPUResourceType = GPUResourceType::Buffer;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorSetIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_DescriptorIndex = 1;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_BindingAccessibility = Accessibility::ReadOnly;
	m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ResourceAccessibility = Accessibility::ReadWrite;
    m_RenderPassComp->m_ResourceBindingLayoutDescs[2].m_ShaderStage = ShaderStage::Vertex;

	m_RenderPassComp->m_ShaderProgram = m_ShaderProgramComp;

	m_debugSphereConstantBuffer.reserve(m_maxDebugMeshes);
	m_debugCubeConstantBuffer.reserve(m_maxDebugMeshes);
	m_debugMaterialConstantBuffer.reserve(m_maxDebugMaterial);

	m_ObjectStatus = ObjectStatus::Created;
	
	return true;
}

bool DebugPass::Initialize()
{

	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_debugSphereMeshGPUBufferComp);
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_debugCubeMeshGPUBufferComp);
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_debugCameraFrustumGPUBufferComp);
	g_Engine->Get<GPUBufferResourceService>()->Initialize(m_debugMaterialGPUBufferComp);

	g_Engine->Get<ShaderProgramResourceService>()->Initialize(m_ShaderProgramComp);
	g_Engine->Get<RenderPassResourceService>()->Initialize(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Activated;

	return true;
}

bool DebugPass::Update()
{
	return true;	
}

bool DebugPass::Terminate()
{

	g_Engine->Get<RenderPassResourceService>()->Delete(m_RenderPassComp);

	m_ObjectStatus = ObjectStatus::Terminated;

	return true;
}

ObjectStatus DebugPass::GetStatus()
{
	return m_ObjectStatus;
}

bool DebugPass::PrepareCommandList(IRenderingContext* renderingContext)
{

	auto l_renderingConfig = g_Engine->Get<RenderingConfigurationService>()->GetRenderingConfig();

	return false;
}

RenderPassComponent* DebugPass::GetRenderPassComp()
{
	return m_RenderPassComp;
}

DebugPerObjectConstantBuffer DebugPass::AddAABB(const AABB& aabb)
{
	DebugPerObjectConstantBuffer l_result;
		
	l_result.m = Math::toTranslationMatrix(aabb.m_center);
	l_result.m.m00 *= aabb.m_extend.x / 2.0f;
	l_result.m.m11 *= aabb.m_extend.y / 2.0f;
	l_result.m.m22 *= aabb.m_extend.z / 2.0f;

	return l_result;
}

bool DebugPass::AddBVHNode(const BVHNode& node)
{

	static bool drawIntermediateBB = false;
	if(node.m_Entity == INVALID_ENTITY && !drawIntermediateBB)
		return true;

	auto l_cubeMeshData = AddAABB(node.m_AABB);
	l_cubeMeshData.materialID = node.m_Entity == INVALID_ENTITY ? 3 : 5;

	m_debugCubeConstantBuffer.emplace_back(l_cubeMeshData);

	return true;
}