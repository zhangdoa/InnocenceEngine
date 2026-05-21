#include "DX12RenderPassResourceService.h"
#include "DX12Context.h"
#include "DX12Helper_Common.h"
#include "DX12Helper_Pipeline.h"
#include "../../Common/LogService.h"
#include "../../Common/LogServiceSpecialization.h"

using namespace Inno;
using namespace DX12Helper;

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
		// A compute pass with no shader program is valid: the pass may own its
		// own root signature + PSO via an external adapter and bind them directly
		// to the command list. Downstream Bind / Dispatch helpers gate on
		// m_PipelineStateObject so skipping PSO creation here is safe.
		if (renderPass->m_ShaderProgram)
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
		else
		{
			Log(Verbose, "Compute PSO skipped for ", renderPass->m_InstanceName.c_str(), " (no shader program — pass owns its own PSOs).");
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
	// PathTracerPayload (source of truth: common/pathTracerPayload.hlsli):
	// hitPos(12) + normal(12) + texCoord(8) + albedo(12) + metalness(4) + roughness(4) + missed(4) + instanceID(4) = 60B.
	// ShadowPayload: 4B. Round up to 64 for 16B alignment.
	shaderConfig.MaxPayloadSizeInBytes = 64;
	shaderConfig.MaxAttributeSizeInBytes = 8; // barycentrics

	D3D12_GLOBAL_ROOT_SIGNATURE globalSig = { PSO->m_RootSignature.Get() };

	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineCfg = {};
	// Depth 2 covers the radiance-cache CHS's nested sky-NEE shadow ray. The PT
	// bounce loop is iterative-from-raygen, so this is a no-op there.
	pipelineCfg.MaxTraceRecursionDepth = 2;

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

	// Shader-table layout (must match DispatchRays packing):
	//   no shadow miss: [RayGen][Miss][HitGroup]
	//   with shadow miss: [RayGen][Miss][ShadowMiss][HitGroup]
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
