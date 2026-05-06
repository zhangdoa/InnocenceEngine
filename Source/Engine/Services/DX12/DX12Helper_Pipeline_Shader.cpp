#include "DX12Helper_Pipeline.h"
#include "../../Common/LogServiceSpecialization.h"
#include "../../Common/IOService.h"
#include "../../Engine.h"

using namespace Inno;

namespace
{
#ifdef USE_DXIL
	const char* m_shaderRelativePath = "Shaders//DXIL//";
#else
	const wchar_t* m_shaderRelativePath = L"Shaders//HLSL//";
#endif
}

bool DX12Helper::LoadGraphicsShaders(RenderPassComponent* RenderPassComp)
{
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(RenderPassComp->m_PipelineStateObject);
	auto l_SPC = RenderPassComp->m_ShaderProgram;

	if (!l_SPC || !l_PSO)
	{
		Log(Verbose, "Skipping creating graphics shaders for ", RenderPassComp->m_InstanceName.c_str());
		return true;
	}

	if (l_SPC->m_VSBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_VSBytecode;
		l_VSBytecode.pShaderBytecode = l_SPC->m_VSBuffer.data();
		l_VSBytecode.BytecodeLength = l_SPC->m_VSBuffer.size();
		l_PSO->m_GraphicsPSODesc.VS = l_VSBytecode;

		Log(Verbose, "Vertex Shader: ", l_SPC->m_VSBuffer.size(), " bytes");
	}
	if (l_SPC->m_HSBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_HSBytecode;
		l_HSBytecode.pShaderBytecode = l_SPC->m_HSBuffer.data();
		l_HSBytecode.BytecodeLength = l_SPC->m_HSBuffer.size();
		l_PSO->m_GraphicsPSODesc.HS = l_HSBytecode;

		Log(Verbose, "Hull Shader: ", l_SPC->m_HSBuffer.size(), " bytes");
	}
	if (l_SPC->m_DSBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_DSBytecode;
		l_DSBytecode.pShaderBytecode = l_SPC->m_DSBuffer.data();
		l_DSBytecode.BytecodeLength = l_SPC->m_DSBuffer.size();
		l_PSO->m_GraphicsPSODesc.DS = l_DSBytecode;

		Log(Verbose, "Domain Shader: ", l_SPC->m_DSBuffer.size(), " bytes");
	}
	if (l_SPC->m_GSBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_GSBytecode;
		l_GSBytecode.pShaderBytecode = l_SPC->m_GSBuffer.data();
		l_GSBytecode.BytecodeLength = l_SPC->m_GSBuffer.size();
		l_PSO->m_GraphicsPSODesc.GS = l_GSBytecode;

		Log(Verbose, "Geometry Shader: ", l_SPC->m_GSBuffer.size(), " bytes");
	}
	if (l_SPC->m_PSBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_PSBytecode;
		l_PSBytecode.pShaderBytecode = l_SPC->m_PSBuffer.data();
		l_PSBytecode.BytecodeLength = l_SPC->m_PSBuffer.size();
		l_PSO->m_GraphicsPSODesc.PS = l_PSBytecode;

		Log(Verbose, "Pixel Shader: ", l_SPC->m_PSBuffer.size(), " bytes");
	}

	return true;
}

bool DX12Helper::LoadComputeShaders(RenderPassComponent* RenderPassComp)
{
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(RenderPassComp->m_PipelineStateObject);
	auto l_SPC = RenderPassComp->m_ShaderProgram;

	if (!l_SPC || !l_PSO)
	{
		Log(Verbose, "Skipping creating ShaderPrograms for ", RenderPassComp->m_InstanceName.c_str());
		return true;
	}

	if (l_SPC->m_CSBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_CSBytecode;
		l_CSBytecode.pShaderBytecode = l_SPC->m_CSBuffer.data();
		l_CSBytecode.BytecodeLength = l_SPC->m_CSBuffer.size();
		l_PSO->m_ComputePSODesc.CS = l_CSBytecode;

		Log(Verbose, "Compute Shader: ", l_SPC->m_CSBuffer.size(), " bytes");
	}

	return true;
}

bool DX12Helper::LoadRaytracingShaders(RenderPassComponent* RenderPassComp)
{
	auto l_PSO = reinterpret_cast<DX12PipelineStateObject*>(RenderPassComp->m_PipelineStateObject);
	auto l_SPC = RenderPassComp->m_ShaderProgram;

	if (!l_SPC || !l_PSO)
	{
		Log(Verbose, "Skipping creating ShaderPrograms for ", RenderPassComp->m_InstanceName.c_str());
		return true;
	}

#ifdef USE_DXIL
	if (l_SPC->m_RayGenBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_LibBytecode;
		l_LibBytecode.pShaderBytecode = l_SPC->m_RayGenBuffer.data();
		l_LibBytecode.BytecodeLength = l_SPC->m_RayGenBuffer.size();

		Log(Verbose, "RayGen Shader: ", l_SPC->m_RayGenBuffer.size(), " bytes");
	}
	if (l_SPC->m_ClosestHitBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_LibBytecode;
		l_LibBytecode.pShaderBytecode = l_SPC->m_ClosestHitBuffer.data();
		l_LibBytecode.BytecodeLength = l_SPC->m_ClosestHitBuffer.size();

		Log(Verbose, "ClosestHit Shader: ", l_SPC->m_ClosestHitBuffer.size(), " bytes");
	}
	if (l_SPC->m_MissBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_LibBytecode;
		l_LibBytecode.pShaderBytecode = l_SPC->m_MissBuffer.data();
		l_LibBytecode.BytecodeLength = l_SPC->m_MissBuffer.size();

		Log(Verbose, "Miss Shader: ", l_SPC->m_MissBuffer.size(), " bytes");
	}
	if (l_SPC->m_AnyHitBuffer.size())
	{
		D3D12_SHADER_BYTECODE l_LibBytecode;
		l_LibBytecode.pShaderBytecode = l_SPC->m_AnyHitBuffer.data();
		l_LibBytecode.BytecodeLength = l_SPC->m_AnyHitBuffer.size();

		Log(Verbose, "AnyHit Shader: ", l_SPC->m_AnyHitBuffer.size(), " bytes");
	}
#endif

	return true;
}

#ifdef USE_DXIL
bool DX12Helper::LoadShaderFile(std::vector<uint8_t> &rhs, const ShaderFilePath &shaderFilePath)
{
	auto l_path = std::string(m_shaderRelativePath) + shaderFilePath.c_str() + ".dxil";
	auto l_rawData = g_Engine->Get<IOService>()->loadFile(l_path.c_str(), IOMode::Binary);
	rhs.resize(l_rawData.size());
	std::memcpy(rhs.data(), l_rawData.data(), l_rawData.size());
	return true;
}
#else
bool DX12Helper::LoadShaderFile(ID3D10Blob** rhs, ShaderStage shaderStage, const ShaderFilePath& shaderFilePath)
{
	const char* l_shaderTypeName;

	switch (shaderStage)
	{
	case ShaderStage::Vertex: l_shaderTypeName = "vs_6_3";
		break;
	case ShaderStage::Hull: l_shaderTypeName = "hs_6_3";
		break;
	case ShaderStage::Domain: l_shaderTypeName = "ds_6_3";
		break;
	case ShaderStage::Geometry: l_shaderTypeName = "gs_6_3";
		break;
	case ShaderStage::Pixel: l_shaderTypeName = "ps_6_3";
		break;
	case ShaderStage::Compute: l_shaderTypeName = "cs_6_3";
		break;
	case ShaderStage::RayGen: l_shaderTypeName = "lib_6_3";
		break;
	case ShaderStage::AnyHit: l_shaderTypeName = "lib_6_3";
		break;
	case ShaderStage::ClosestHit: l_shaderTypeName = "lib_6_3";
		break;
	case ShaderStage::Miss: l_shaderTypeName = "lib_6_3";
		break;
	default:
		break;
	}

#if defined(INNO_DEBUG)
	// Enable better shader debugging with the graphics debugging tools.
	UINT l_compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
	UINT l_compileFlags = 0;
#endif

	ComPtr<ID3D10Blob> l_errorMessage = 0;
	auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
	auto l_workingDirW = std::wstring(l_workingDir.begin(), l_workingDir.end());
	auto l_shadeFilePathW = std::wstring(shaderFilePath.begin(), shaderFilePath.end());
	auto l_HResult = D3DCompileFromFile((l_workingDirW + m_shaderRelativePath + l_shadeFilePathW).c_str(), NULL, D3D_COMPILE_STANDARD_FILE_INCLUDE, "main", l_shaderTypeName, l_compileFlags, 0, rhs, &l_errorMessage);

	if (FAILED(l_HResult))
	{
		if (l_errorMessage)
		{
			auto l_errorMessagePtr = (char*)(l_errorMessage->GetBufferPointer());
			auto bufferSize = l_errorMessage->GetBufferSize();
			std::vector<char> l_errorMessageVector(bufferSize);
			std::memcpy(l_errorMessageVector.data(), l_errorMessagePtr, bufferSize);

			Log(Error, "", shaderFilePath.c_str(), " compile error: ", &l_errorMessageVector[0], "\n -- --------------------------------------------------- -- ");
		}
		else
		{
			Log(Error, "Can't find ", shaderFilePath.c_str(), " ", name);
		}
		return false;
	}

	Log(Verbose, "", shaderFilePath.c_str(), " has been compiled.");
	return true;
}
#endif
