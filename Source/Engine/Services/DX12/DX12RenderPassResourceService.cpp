#include "DX12RenderPassResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "DX12Helper_Texture.h"
#include "../../Engine.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../FrameManagementService.h"
#include "../TextureResourceService.h"

using namespace Inno;
using namespace DX12Helper;

// ---------------------------------------------------------------------------
// Setup / Terminate
// ---------------------------------------------------------------------------

bool DX12RenderPassResourceService::Setup(IServiceConfig* systemConfig)
{
	RenderPassResourceService::Setup(systemConfig);

	m_PSOPool = TObjectPool<DX12PipelineStateObject>::Create(128);
	m_SemaphorePool = TObjectPool<DX12Semaphore>::Create(256);
	m_OutputMergerTargetPool = TObjectPool<DX12OutputMergerTarget>::Create(128);

	return true;
}

bool DX12RenderPassResourceService::Terminate()
{
	delete m_PSOPool;
	delete m_SemaphorePool;
	delete m_OutputMergerTargetPool;

	RenderPassResourceService::Terminate();

	return true;
}

// ---------------------------------------------------------------------------
// Pool allocation
// ---------------------------------------------------------------------------

IPipelineStateObject* DX12RenderPassResourceService::AddPipelineStateObject()
{
	return m_PSOPool->Spawn();
}

ISemaphore* DX12RenderPassResourceService::AddSemaphore()
{
	return m_SemaphorePool->Spawn();
}

bool DX12RenderPassResourceService::Add(IOutputMergerTarget*& rhs)
{
	rhs = m_OutputMergerTargetPool->Spawn();
	return rhs != nullptr;
}

// ---------------------------------------------------------------------------
// Deletion
// ---------------------------------------------------------------------------

bool DX12RenderPassResourceService::Delete(RenderPassComponent* ptr)
{
	return RenderPassResourceService::Delete(ptr);
}

bool DX12RenderPassResourceService::Delete(IPipelineStateObject* rhs)
{
	auto l_rhs = reinterpret_cast<DX12PipelineStateObject*>(rhs);
	l_rhs->m_PSO.Reset();
	m_PSOPool->Destroy(l_rhs);
	return true;
}

bool DX12RenderPassResourceService::Delete(ISemaphore* rhs)
{
	auto l_rhs = reinterpret_cast<DX12Semaphore*>(rhs);
	m_SemaphorePool->Destroy(l_rhs);
	return true;
}

bool DX12RenderPassResourceService::Delete(IOutputMergerTarget* rhs)
{
	auto l_rhs = reinterpret_cast<DX12OutputMergerTarget*>(rhs);
	auto l_textureService = g_Engine->Get<TextureResourceService>();

	for (auto& j : l_rhs->m_ColorOutputs)
	{
		if (j)
			l_textureService->Delete(j);
	}

	l_rhs->m_ColorOutputs.clear();

	if (l_rhs->m_DepthStencilOutput)
		l_textureService->Delete(l_rhs->m_DepthStencilOutput);

	l_rhs->m_DepthStencilOutput = nullptr;

	m_OutputMergerTargetPool->Destroy(l_rhs);

	return true;
}

// ---------------------------------------------------------------------------
// ReadRenderTargetSample
// ---------------------------------------------------------------------------

Vec4 DX12RenderPassResourceService::ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

// ---------------------------------------------------------------------------
// Root signature
// ---------------------------------------------------------------------------

bool DX12RenderPassResourceService::CreateRootSignature(RenderPassComponent* RenderPassComp)
{
	if (RenderPassComp->m_ResourceBindingLayoutDescs.empty())
		Log(Verbose, "Creating empty RootSignature for ", RenderPassComp->m_InstanceName);

	auto l_maxBindingCount = RenderPassComp->m_ResourceBindingLayoutDescs.size();
	std::vector<CD3DX12_ROOT_PARAMETER1> l_rootParameters;
	l_rootParameters.reserve(l_maxBindingCount);

	std::vector<D3D12_DESCRIPTOR_RANGE1> l_descriptorRanges;
	l_descriptorRanges.reserve(l_maxBindingCount);

	for (size_t i = 0; i < l_maxBindingCount; i++)
	{
		auto& l_resourceBinderLayoutDesc = RenderPassComp->m_ResourceBindingLayoutDescs[i];
		auto l_descriptorRange = GetDescriptorRange(RenderPassComp, l_resourceBinderLayoutDesc);
		CD3DX12_ROOT_PARAMETER1 l_rootParameter = {};
		if (l_descriptorRange.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_CBV)
		{
			if (l_resourceBinderLayoutDesc.m_IsRootConstant)
			{
				Log(Verbose, RenderPassComp->m_InstanceName, " Root Constant: at root parameter ", i,
					" with ", l_resourceBinderLayoutDesc.m_SubresourceCount, " constants.");
				l_rootParameter.InitAsConstants(l_resourceBinderLayoutDesc.m_SubresourceCount, l_resourceBinderLayoutDesc.m_DescriptorIndex);
			}
			else
			{
				Log(Verbose, RenderPassComp->m_InstanceName, " Root CBV: at root parameter ", i, " with ", l_descriptorRange.NumDescriptors, " descriptors.");
				l_rootParameter.InitAsConstantBufferView(l_resourceBinderLayoutDesc.m_DescriptorIndex);
			}
		}
		else
		{
			const char* rangeTypeName = "";
			auto& l_lastRange = l_descriptorRanges.emplace_back(l_descriptorRange);
			switch (l_descriptorRange.RangeType)
			{
			case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
			{
				l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
				rangeTypeName = "Root Descriptor Table CBV";
				break;
			}
			case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
			{
				if (l_resourceBinderLayoutDesc.m_GPUBufferUsage == GPUBufferUsage::TLAS)
				{
					l_rootParameter.InitAsShaderResourceView(l_resourceBinderLayoutDesc.m_DescriptorIndex);
					rangeTypeName = "Root Shader Resource View";
				}
				else
				{
					l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
					rangeTypeName = "Root Descriptor Table SRV";
				}
				break;
			}
			case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
			{
				l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
				rangeTypeName = "Root Descriptor Table UAV";
				break;
			}
			case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
			{
				l_rootParameter.InitAsDescriptorTable(1, &l_lastRange);
				rangeTypeName = "Root Descriptor Table Sampler";
				break;
			}
			}
			Log(Verbose, RenderPassComp->m_InstanceName, ": ", rangeTypeName, " at root parameter ", i,
				" BaseShaderRegister ", l_descriptorRange.BaseShaderRegister, " with ", l_descriptorRange.NumDescriptors, " descriptors.");
		}

		l_rootParameters.emplace_back(l_rootParameter);
	}

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC l_rootSigDesc((uint32_t)l_rootParameters.size(), l_rootParameters.data());

	if (RenderPassComp->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics && RenderPassComp->m_RenderPassDesc.m_UseOutputMerger)
	{
		l_rootSigDesc.Desc_1_1.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	}

	ComPtr<ID3DBlob> l_signature = 0;
	ComPtr<ID3DBlob> l_error = 0;

	auto l_HResult = D3D12SerializeVersionedRootSignature(&l_rootSigDesc, &l_signature, &l_error);

	if (FAILED(l_HResult))
	{
		if (l_error)
		{
			auto l_errorMessagePtr = (char*)(l_error->GetBufferPointer());
			auto bufferSize = l_error->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);
			l_error->Release();

			Log(Error, RenderPassComp->m_InstanceName, " RootSignature serialization error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			Log(Error, RenderPassComp->m_InstanceName, " Can't serialize RootSignature.");
		}
		return false;
	}

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(RenderPassComp->m_PipelineStateObject);
	l_HResult = m_ctx->m_device->CreateRootSignature(0, l_signature->GetBufferPointer(), l_signature->GetBufferSize(), IID_PPV_ARGS(&l_PSO->m_RootSignature));

	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_ctx->m_device.Get(), "RootSignature", RenderPassComp->m_InstanceName.c_str(), l_HResult);
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(RenderPassComp, l_PSO->m_RootSignature, "RootSignature");
#endif // INNO_DEBUG

	Log(Verbose, RenderPassComp->m_InstanceName, " RootSignature has been created.");

	if (RenderPassComp->m_RenderPassDesc.m_IndirectDraw)
	{
		D3D12_INDIRECT_ARGUMENT_DESC argumentDescs[4] = {};

		// 1. Constant argument (must be first to match HLSL structure).
		argumentDescs[0].Type = D3D12_INDIRECT_ARGUMENT_TYPE_CONSTANT;
		argumentDescs[0].Constant.RootParameterIndex = 0; // The root signature slot for the constant.
		argumentDescs[0].Constant.DestOffsetIn32BitValues = 0; // Start at offset 0.
		argumentDescs[0].Constant.Num32BitValuesToSet = 2; // Number of 32-bit values. Because of the 8-byte alignment requirement, we put two 32-bit values here.

		// 2. Vertex buffer view.
		argumentDescs[1].Type = D3D12_INDIRECT_ARGUMENT_TYPE_VERTEX_BUFFER_VIEW;

		// 3. Index buffer view.
		argumentDescs[2].Type = D3D12_INDIRECT_ARGUMENT_TYPE_INDEX_BUFFER_VIEW;

		// 4. Draw indexed arguments (must be last).
		argumentDescs[3].Type = D3D12_INDIRECT_ARGUMENT_TYPE_DRAW_INDEXED;

		D3D12_COMMAND_SIGNATURE_DESC commandSignatureDesc = {};
		commandSignatureDesc.NumArgumentDescs = _countof(argumentDescs);
		commandSignatureDesc.pArgumentDescs = argumentDescs;
		commandSignatureDesc.ByteStride = 64;

		l_HResult = m_ctx->m_device->CreateCommandSignature(&commandSignatureDesc, l_PSO->m_RootSignature.Get(), IID_PPV_ARGS(&l_PSO->m_IndirectCommandSignature));
		if (FAILED(l_HResult))
		{
			LogD3D12CreateFailure(m_ctx->m_device.Get(), "CommandSignature", RenderPassComp->m_InstanceName.c_str(), l_HResult);
			return false;
		}

		Log(Verbose, RenderPassComp->m_InstanceName, " CommandSignature has been created.");
	}

	return true;
}

// ---------------------------------------------------------------------------
// Descriptor range
// ---------------------------------------------------------------------------

D3D12_DESCRIPTOR_RANGE1 DX12RenderPassResourceService::GetDescriptorRange(RenderPassComponent* RenderPassComp, const ResourceBindingLayoutDesc& resourceBinderLayoutDesc)
{
	auto& l_descriptorAccessor = m_ctx->GetDescriptorHeapAccessor(resourceBinderLayoutDesc.m_GPUResourceType, resourceBinderLayoutDesc.m_BindingAccessibility
		, resourceBinderLayoutDesc.m_ResourceAccessibility, resourceBinderLayoutDesc.m_TextureUsage);

	D3D12_DESCRIPTOR_RANGE1 l_range = {};
	if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
	{
		l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	}

	if (resourceBinderLayoutDesc.m_BindingAccessibility == Accessibility::ReadOnly)
	{
		if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
		{
			if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
			{
				l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
			}
			else if (resourceBinderLayoutDesc.m_ResourceAccessibility.CanWrite())
			{
				l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
			}
		}
		else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
		{
			if (resourceBinderLayoutDesc.m_ResourceAccessibility == Accessibility::ReadOnly)
				l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		}
	}
	else if (resourceBinderLayoutDesc.m_BindingAccessibility.CanWrite())
	{
		if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
		{
			l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		}
		else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
		{
			l_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
		}
	}

	l_range.BaseShaderRegister = resourceBinderLayoutDesc.m_DescriptorIndex;

	if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Buffer)
		l_range.NumDescriptors = 1;
	else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Image)
	{
		if (resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::Sample)
			l_range.NumDescriptors = l_descriptorAccessor.GetDesc().m_MaxDescriptors;
		else if (resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::DepthAttachment
			|| resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::DepthStencilAttachment
			|| resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::ColorAttachment
			|| resourceBinderLayoutDesc.m_TextureUsage == TextureUsage::ComputeOnly)
			l_range.NumDescriptors = 1;
	}
	else if (resourceBinderLayoutDesc.m_GPUResourceType == GPUResourceType::Sampler)
		l_range.NumDescriptors = 1;

	return l_range;
}

// ---------------------------------------------------------------------------
// Pipeline state object
// ---------------------------------------------------------------------------

bool DX12RenderPassResourceService::CreatePipelineStateObject(RenderPassComponent* renderPass)
{
	bool l_result = true;
	l_result &= CreateRootSignature(renderPass);

	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(renderPass->m_PipelineStateObject);
	if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Graphics)
	{
		if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
		{
			l_result &= CreateGraphicsPipelineStateObject(renderPass, l_PSO);
		}
	}
	else if (renderPass->m_RenderPassDesc.m_GPUEngineType == GPUEngineType::Compute && !renderPass->m_RenderPassDesc.m_UseRaytracing)
	{
		LoadComputeShaders(renderPass);

		l_PSO->m_ComputePSODesc.pRootSignature = l_PSO->m_RootSignature.Get();
		auto l_HResult = m_ctx->m_device->CreateComputePipelineState(&l_PSO->m_ComputePSODesc, IID_PPV_ARGS(&l_PSO->m_PSO));

		if (FAILED(l_HResult))
		{
			LogD3D12CreateFailure(m_ctx->m_device.Get(), "Compute PSO", renderPass->m_InstanceName.c_str(), l_HResult);
			return false;
		}
	}

	if (l_PSO->m_PSO)
	{
#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
		DX12Helper::SetObjectName(renderPass, l_PSO->m_PSO, "PSO");
#endif // INNO_DEBUG
		Log(Verbose, renderPass->m_InstanceName, " PSO has been created.");
	}

	if (renderPass->m_RenderPassDesc.m_UseRaytracing)
	{
		l_result &= CreateRaytracingPipelineStateObject(renderPass, l_PSO);
	}

	return l_result;
}

bool DX12RenderPassResourceService::CreateGraphicsPipelineStateObject(RenderPassComponent* RenderPassComp, DX12PipelineStateObject* PSO)
{
	GenerateDepthStencilStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc, PSO);
	GenerateBlendStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_BlendDesc, PSO);
	GenerateRasterizerStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_RasterizerDesc, PSO);
	GenerateViewportStateDesc(RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_ViewportDesc, PSO);

	PSO->m_GraphicsPSODesc.NumRenderTargets = (uint32_t)RenderPassComp->m_RenderPassDesc.m_RenderTargetCount;

	auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
	auto l_RTV = l_DX12OutputMergerTarget->m_RTVs[0];
	for (size_t i = 0; i < RenderPassComp->m_RenderPassDesc.m_RenderTargetCount; i++)
	{
		PSO->m_GraphicsPSODesc.RTVFormats[i] = l_RTV.m_Desc.Format;
	}

	if (RenderPassComp->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		auto l_DX12OutputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(RenderPassComp->m_OutputMergerTarget);
		auto l_DSV = l_DX12OutputMergerTarget->m_DSVs[0];
		PSO->m_GraphicsPSODesc.DSVFormat = l_DSV.m_Desc.Format;
		PSO->m_GraphicsPSODesc.DepthStencilState = PSO->m_DepthStencilDesc;
	}

	PSO->m_GraphicsPSODesc.RasterizerState = PSO->m_RasterizerDesc;
	PSO->m_GraphicsPSODesc.BlendState = PSO->m_BlendDesc;
	PSO->m_GraphicsPSODesc.SampleMask = UINT_MAX;
	PSO->m_GraphicsPSODesc.PrimitiveTopologyType = PSO->m_PrimitiveTopologyType;
	PSO->m_GraphicsPSODesc.SampleDesc.Count = 1;
	if (!PSO->m_RootSignature.Get() || !RenderPassComp->m_ShaderProgram)
	{
		Log(Verbose, "Skipping creating Graphics PSO for ", RenderPassComp->m_InstanceName);
		return true;
	}

	PSO->m_GraphicsPSODesc.pRootSignature = PSO->m_RootSignature.Get();

	CreateInputLayout(PSO);
	LoadGraphicsShaders(RenderPassComp);

	auto l_HResult = m_ctx->m_device->CreateGraphicsPipelineState(&PSO->m_GraphicsPSODesc, IID_PPV_ARGS(&PSO->m_PSO));
	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_ctx->m_device.Get(), "Graphics PSO", RenderPassComp->m_InstanceName.c_str(), l_HResult);
		return false;
	}

	return true;
}

bool DX12RenderPassResourceService::CreateRaytracingPipelineStateObject(RenderPassComponent* RenderPassComp, DX12PipelineStateObject* PSO)
{
	auto l_SPC = RenderPassComp->m_ShaderProgram;

	if (!PSO->m_RootSignature)
	{
		Log(Error, RenderPassComp->m_InstanceName, " Global root signature is null!");
		return false;
	}

	LoadRaytracingShaders(RenderPassComp);

	const bool hasShadowMiss = !l_SPC->m_ShadowMissBuffer.empty();

	D3D12_DXIL_LIBRARY_DESC rayGenLib = {};
	rayGenLib.DXILLibrary.pShaderBytecode = &l_SPC->m_RayGenBuffer[0];
	rayGenLib.DXILLibrary.BytecodeLength = l_SPC->m_RayGenBuffer.size();

	D3D12_DXIL_LIBRARY_DESC closestHitLib = {};
	closestHitLib.DXILLibrary.pShaderBytecode = &l_SPC->m_ClosestHitBuffer[0];
	closestHitLib.DXILLibrary.BytecodeLength = l_SPC->m_ClosestHitBuffer.size();

	D3D12_DXIL_LIBRARY_DESC anyHitLib = {};
	anyHitLib.DXILLibrary.pShaderBytecode = &l_SPC->m_AnyHitBuffer[0];
	anyHitLib.DXILLibrary.BytecodeLength = l_SPC->m_AnyHitBuffer.size();

	D3D12_DXIL_LIBRARY_DESC missLib = {};
	missLib.DXILLibrary.pShaderBytecode = &l_SPC->m_MissBuffer[0];
	missLib.DXILLibrary.BytecodeLength = l_SPC->m_MissBuffer.size();

	D3D12_DXIL_LIBRARY_DESC shadowMissLib = {};
	if (hasShadowMiss)
	{
		shadowMissLib.DXILLibrary.pShaderBytecode = &l_SPC->m_ShadowMissBuffer[0];
		shadowMissLib.DXILLibrary.BytecodeLength = l_SPC->m_ShadowMissBuffer.size();
	}

	D3D12_HIT_GROUP_DESC hitGroupDesc = {};
	hitGroupDesc.HitGroupExport = L"HitGroup";
	hitGroupDesc.Type = D3D12_HIT_GROUP_TYPE_TRIANGLES;
	hitGroupDesc.ClosestHitShaderImport = L"ClosestHitShader";
	hitGroupDesc.AnyHitShaderImport = L"AnyHitShader";
	hitGroupDesc.IntersectionShaderImport = nullptr;

	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	// PathTracerPayload: hitPos(12) + normal(12) + texCoord(8) + albedo(12) + metalness(4) + roughness(4) + missed(4) = 56B
	// ShadowPayload: 4B. Round up to 64 for alignment headroom.
	shaderConfig.MaxPayloadSizeInBytes = 64;
	shaderConfig.MaxAttributeSizeInBytes = 8; // barycentrics

	D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { PSO->m_RootSignature.Get() };

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = {};
	pipelineCfg.MaxTraceRecursionDepth = 1;

	// Up to 9 subobjects: RayGen + ClosestHit + AnyHit + Miss + (opt ShadowMiss) + HitGroup + ShaderConfig + GlobalRS + PipelineCfg
	D3D12_STATE_SUBOBJECT subobjects[9] = {};
	uint32_t subIdx = 0;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &rayGenLib;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &closestHitLib;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &anyHitLib;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
	subobjects[subIdx++].pDesc = &missLib;

	if (hasShadowMiss)
	{
		subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY;
		subobjects[subIdx++].pDesc = &shadowMissLib;
	}

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP;
	subobjects[subIdx++].pDesc = &hitGroupDesc;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG;
	subobjects[subIdx++].pDesc = &shaderConfig;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE;
	subobjects[subIdx++].pDesc = &globalSig;

	subobjects[subIdx].Type = D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG;
	subobjects[subIdx++].pDesc = &pipelineCfg;

	D3D12_STATE_OBJECT_DESC stateObjectDesc = {};
	stateObjectDesc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;
	stateObjectDesc.NumSubobjects = subIdx;
	stateObjectDesc.pSubobjects = subobjects;

	HRESULT l_HResult = m_ctx->m_device->CreateStateObject(&stateObjectDesc, IID_PPV_ARGS(&PSO->m_RaytracingPSO));
	if (FAILED(l_HResult))
	{
		LogD3D12CreateFailure(m_ctx->m_device.Get(), "Raytracing PSO", RenderPassComp->m_InstanceName.c_str(), l_HResult);
		return false;
	}

#if defined(INNO_DEBUG) || defined(INNO_RELWITHDEBINFO)
	DX12Helper::SetObjectName(RenderPassComp, PSO->m_RaytracingPSO, "RaytracingPSO");
#endif

	Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing PSO has been created.");

	// Shader table layout:
	//   hasShadowMiss == false: [RayGen][Miss][HitGroup]             (3 slots)
	//   hasShadowMiss == true:  [RayGen][Miss][ShadowMiss][HitGroup] (4 slots)
	const uint32_t numSlots = hasShadowMiss ? 4 : 3;
	PSO->m_RaytracingMissShaderCount = hasShadowMiss ? 2u : 1u;
	PSO->m_RaytracingHitGroupCount   = 1u;
	auto l_shaderIDBufferSize = numSlots * D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	auto l_shaderIDBufferDesc = CD3DX12_RESOURCE_DESC::Buffer(l_shaderIDBufferSize);
	PSO->m_RaytracingShaderIDBuffer = m_ctx->CreateUploadHeapBuffer(&l_shaderIDBufferDesc);

	ID3D12StateObjectProperties* props;
	PSO->m_RaytracingPSO->QueryInterface(&props);

	void* data;
	auto writeId = [&](const wchar_t* name) {
		void* id = props->GetShaderIdentifier(name);
		memcpy(data, id, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
		data = static_cast<char*>(data) + D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	};

	PSO->m_RaytracingShaderIDBuffer->Map(0, nullptr, &data);
	writeId(L"RayGenShader");
	writeId(L"MissShader");
	if (hasShadowMiss)
		writeId(L"ShadowMissShader");
	writeId(L"HitGroup");
	PSO->m_RaytracingShaderIDBuffer->Unmap(0, nullptr);
	props->Release();

	Log(Verbose, RenderPassComp->m_InstanceName, " Raytracing shader IDs have been written.");

	return true;
}

// ---------------------------------------------------------------------------
// Fence events
// ---------------------------------------------------------------------------

bool DX12RenderPassResourceService::CreateFenceEvents(RenderPassComponent* renderPass)
{
	bool result = true;
	for (size_t i = 0; i < renderPass->m_Semaphores.size(); i++)
	{
		auto l_semaphore = reinterpret_cast<DX12Semaphore*>(renderPass->m_Semaphores[i]);
		l_semaphore->m_DirectCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_DirectCommandQueueFenceEvent == NULL)
		{
			Log(Error, renderPass->m_InstanceName, " Can't create fence event for direct CommandQueue.");
			result = false;
		}

		l_semaphore->m_ComputeCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_ComputeCommandQueueFenceEvent == NULL)
		{
			Log(Error, renderPass->m_InstanceName, " Can't create fence event for compute CommandQueue.");
			result = false;
		}

		l_semaphore->m_CopyCommandQueueFenceEvent = CreateEventEx(NULL, FALSE, FALSE, EVENT_ALL_ACCESS);
		if (l_semaphore->m_CopyCommandQueueFenceEvent == NULL)
		{
			Log(Error, renderPass->m_InstanceName, " Can't create fence event for copy CommandQueue.");
			result = false;
		}
	}

	if (result)
	{
		Log(Verbose, renderPass->m_InstanceName, " Fence events have been created.");
	}

	return result;
}

// ---------------------------------------------------------------------------
// Output merger targets
// ---------------------------------------------------------------------------

bool DX12RenderPassResourceService::OnOutputMergerTargetsCreated(RenderPassComponent* renderPass)
{
	auto l_outputMergerTarget = reinterpret_cast<DX12OutputMergerTarget*>(renderPass->m_OutputMergerTarget);
	auto l_swapChainImageCount = g_Engine->Get<FrameManagementService>()->GetSwapChainImageCount();

	if (renderPass->m_RenderPassDesc.m_UseOutputMerger)
	{
		auto& l_RTVs = l_outputMergerTarget->m_RTVs;
		if (l_RTVs.size() == 0)
		{
			l_RTVs.resize(l_swapChainImageCount);
			for (size_t i = 0; i < l_RTVs.size(); i++)
			{
				auto& l_RTV = l_RTVs[i];
				l_RTV.m_Desc = GetRTVDesc(renderPass->m_RenderPassDesc.m_RenderTargetDesc);
				l_RTV.m_Handles.resize(renderPass->m_RenderPassDesc.m_RenderTargetCount);
				for (size_t j = 0; j < l_RTV.m_Handles.size(); j++)
				{
					auto l_handle = m_ctx->m_RTVDescHeapAccessor.GetNewHandle();
					l_RTV.m_Handles[j] = D3D12_CPU_DESCRIPTOR_HANDLE{ l_handle.m_CPUHandle };
				}
			}
		}

		for (size_t i = 0; i < l_RTVs.size(); i++)
		{
			auto& l_RTV = l_RTVs[i];
			for (size_t j = 0; j < l_outputMergerTarget->m_ColorOutputs.size(); j++)
			{
				auto l_renderTarget = static_cast<ID3D12Resource*>(l_outputMergerTarget->m_ColorOutputs[j]->GetGPUResource(i));
				m_ctx->m_device->CreateRenderTargetView(l_renderTarget, &l_RTV.m_Desc, l_RTV.m_Handles[j]);
			}
		}
	}

	if (renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_DepthEnable)
	{
		auto& l_DSVs = l_outputMergerTarget->m_DSVs;
		if (l_DSVs.size() == 0)
		{
			l_DSVs.resize(l_swapChainImageCount);
			for (size_t i = 0; i < l_DSVs.size(); i++)
			{
				l_DSVs[i].m_Desc = GetDSVDesc(renderPass->m_RenderPassDesc.m_RenderTargetDesc, renderPass->m_RenderPassDesc.m_GraphicsPipelineDesc.m_DepthStencilDesc.m_StencilEnable);
				l_DSVs[i].m_Handle = D3D12_CPU_DESCRIPTOR_HANDLE{ m_ctx->m_DSVDescHeapAccessor.GetNewHandle().m_CPUHandle };
			}
		}

		auto l_renderTargetTexture = reinterpret_cast<TextureComponent*>(l_outputMergerTarget->m_DepthStencilOutput);
		for (size_t i = 0; i < l_DSVs.size(); i++)
		{
			auto l_renderTarget = static_cast<ID3D12Resource*>(l_outputMergerTarget->m_DepthStencilOutput->GetGPUResource(i));
			m_ctx->m_device->CreateDepthStencilView(l_renderTarget, &l_DSVs[i].m_Desc, l_DSVs[i].m_Handle);
		}
	}

	return true;
}
