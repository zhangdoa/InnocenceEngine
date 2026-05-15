#include "DX12ShaderProgramResourceService.h"
#include "DX12Helper_Pipeline.h"

using namespace Inno;
using namespace DX12Helper;

bool DX12ShaderProgramResourceService::InitializeImpl(ShaderProgramComponent* shaderProgram)
{
#ifdef USE_DXIL
	if (shaderProgram->m_ShaderFilePaths.m_VSPath != "")
	{
		LoadShaderFile(shaderProgram->m_VSBuffer, shaderProgram->m_ShaderFilePaths.m_VSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_HSPath != "")
	{
		LoadShaderFile(shaderProgram->m_HSBuffer, shaderProgram->m_ShaderFilePaths.m_HSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_DSPath != "")
	{
		LoadShaderFile(shaderProgram->m_DSBuffer, shaderProgram->m_ShaderFilePaths.m_DSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_GSPath != "")
	{
		LoadShaderFile(shaderProgram->m_GSBuffer, shaderProgram->m_ShaderFilePaths.m_GSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_PSPath != "")
	{
		LoadShaderFile(shaderProgram->m_PSBuffer, shaderProgram->m_ShaderFilePaths.m_PSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_CSPath != "")
	{
		LoadShaderFile(shaderProgram->m_CSBuffer, shaderProgram->m_ShaderFilePaths.m_CSPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_RayGenPath != "")
	{
		LoadShaderFile(shaderProgram->m_RayGenBuffer, shaderProgram->m_ShaderFilePaths.m_RayGenPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		LoadShaderFile(shaderProgram->m_AnyHitBuffer, shaderProgram->m_ShaderFilePaths.m_AnyHitPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		LoadShaderFile(shaderProgram->m_ClosestHitBuffer, shaderProgram->m_ShaderFilePaths.m_ClosestHitPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_MissPath != "")
	{
		LoadShaderFile(shaderProgram->m_MissBuffer, shaderProgram->m_ShaderFilePaths.m_MissPath);
	}
	if (shaderProgram->m_ShaderFilePaths.m_ShadowMissPath != "")
	{
		LoadShaderFile(shaderProgram->m_ShadowMissBuffer, shaderProgram->m_ShaderFilePaths.m_ShadowMissPath);
	}
#else
	ComPtr<ID3DBlob> tempBuffer;
	if (shaderProgram->m_ShaderFilePaths.m_VSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Vertex, shaderProgram->m_ShaderFilePaths.m_VSPath))
		{
			shaderProgram->m_VSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_VSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_HSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Hull, shaderProgram->m_ShaderFilePaths.m_HSPath))
		{
			shaderProgram->m_HSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_HSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_DSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Domain, shaderProgram->m_ShaderFilePaths.m_DSPath))
		{
			shaderProgram->m_DSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_DSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_GSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Geometry, shaderProgram->m_ShaderFilePaths.m_GSPath))
		{
			shaderProgram->m_GSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_GSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_PSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Pixel, shaderProgram->m_ShaderFilePaths.m_PSPath))
		{
			shaderProgram->m_PSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_PSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_CSPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Compute, shaderProgram->m_ShaderFilePaths.m_CSPath))
		{
			shaderProgram->m_CSBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_CSBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_RayGenPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::RayGen, shaderProgram->m_ShaderFilePaths.m_RayGenPath))
		{
			shaderProgram->m_RayGenBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_RayGenBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_AnyHitPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::AnyHit, shaderProgram->m_ShaderFilePaths.m_AnyHitPath))
		{
			shaderProgram->m_AnyHitBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_AnyHitBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_ClosestHitPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::ClosestHit, shaderProgram->m_ShaderFilePaths.m_ClosestHitPath))
		{
			shaderProgram->m_ClosestHitBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_ClosestHitBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_MissPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Miss, shaderProgram->m_ShaderFilePaths.m_MissPath))
		{
			shaderProgram->m_MissBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_MissBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
	if (shaderProgram->m_ShaderFilePaths.m_ShadowMissPath != "")
	{
		if (LoadShaderFile(&tempBuffer, ShaderStage::Miss, shaderProgram->m_ShaderFilePaths.m_ShadowMissPath))
		{
			shaderProgram->m_ShadowMissBuffer.resize(tempBuffer->GetBufferSize());
			std::memcpy(shaderProgram->m_ShadowMissBuffer.data(), tempBuffer->GetBufferPointer(), tempBuffer->GetBufferSize());
		}
	}
#endif
	shaderProgram->m_ObjectStatus = ObjectStatus::Activated;

	return true;
}
