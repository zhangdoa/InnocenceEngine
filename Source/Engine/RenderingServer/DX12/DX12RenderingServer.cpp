#include "DX12RenderingServer.h"

bool DX12RenderingServer::Setup()
{
	return true;
}

bool DX12RenderingServer::Initialize()
{
	return true;
}

bool DX12RenderingServer::Terminate()
{
	return true;
}

ObjectStatus DX12RenderingServer::GetStatus()
{
	return ObjectStatus();
}

MeshDataComponent * DX12RenderingServer::AddMeshDataComponent(const char * name)
{
	return nullptr;
}

TextureDataComponent * DX12RenderingServer::AddTextureDataComponent(const char * name)
{
	return nullptr;
}

MaterialDataComponent * DX12RenderingServer::AddMaterialDataComponent(const char * name)
{
	return nullptr;
}

RenderPassDataComponent * DX12RenderingServer::AddRenderPassDataComponent(const char * name)
{
	return nullptr;
}

ShaderProgramComponent * DX12RenderingServer::AddShaderProgramComponent(const char * name)
{
	return nullptr;
}

GPUBufferDataComponent * DX12RenderingServer::AddGPUBufferDataComponent(const char * name)
{
	return nullptr;
}

bool DX12RenderingServer::InitializeMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return false;
}

bool DX12RenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue)
{
	return true;
}

bool DX12RenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX12RenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::BindGPUBufferDataComponent(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent * rhs, size_t startOffset, size_t range)
{
	return true;
}

bool DX12RenderingServer::BindShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::BindMaterialDataComponent(ShaderType shaderType, MaterialDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::DispatchDrawCall(RenderPassDataComponent* renderPass, MeshDataComponent* mesh)
{
	return true;
}

bool DX12RenderingServer::UnbindMaterialDataComponent(ShaderType shaderType, MaterialDataComponent * rhs)
{
	return true;
}

bool DX12RenderingServer::CommandListEnd(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX12RenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX12RenderingServer::WaitForFrame(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX12RenderingServer::Present()
{
	return true;
}

bool DX12RenderingServer::CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool DX12RenderingServer::CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool DX12RenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	return true;
}

vec4 DX12RenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return vec4();
}

std::vector<vec4> DX12RenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	return std::vector<vec4>();
}

bool DX12RenderingServer::Resize()
{
	return true;
}

bool DX12RenderingServer::ReloadShader(RenderPassType renderPassType)
{
	return true;
}

bool DX12RenderingServer::BakeGIData()
{
	return true;
}