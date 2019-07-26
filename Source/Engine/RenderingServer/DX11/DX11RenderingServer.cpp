#include "DX11RenderingServer.h"

bool DX11RenderingServer::Setup()
{
	return true;
}

bool DX11RenderingServer::Initialize()
{
	return true;
}

bool DX11RenderingServer::Terminate()
{
	return true;
}

ObjectStatus DX11RenderingServer::GetStatus()
{
	return ObjectStatus();
}

MeshDataComponent * DX11RenderingServer::AddMeshDataComponent(const char * name)
{
	return nullptr;
}

TextureDataComponent * DX11RenderingServer::AddTextureDataComponent(const char * name)
{
	return nullptr;
}

MaterialDataComponent * DX11RenderingServer::AddMaterialDataComponent(const char * name)
{
	return nullptr;
}

RenderPassDataComponent * DX11RenderingServer::AddRenderPassDataComponent(const char * name)
{
	return nullptr;
}

ShaderProgramComponent * DX11RenderingServer::AddShaderProgramComponent(const char * name)
{
	return nullptr;
}

GPUBufferDataComponent * DX11RenderingServer::AddGPUBufferDataComponent(const char * name)
{
	return nullptr;
}

bool DX11RenderingServer::InitializeMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return false;
}

bool DX11RenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue)
{
	return true;
}

bool DX11RenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX11RenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::BindGPUBufferDataComponent(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent * GPUBufferDataComponent, size_t startOffset, size_t range)
{
	return true;
}

bool DX11RenderingServer::BindShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::BindMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::DispatchDrawCall(RenderPassDataComponent* renderPass, MeshDataComponent* mesh)
{
	return true;
}

bool DX11RenderingServer::UnbindMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool DX11RenderingServer::CommandListEnd(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX11RenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX11RenderingServer::WaitForFrame(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool DX11RenderingServer::Present()
{
	return true;
}

bool DX11RenderingServer::CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool DX11RenderingServer::CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool DX11RenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	return true;
}

vec4 DX11RenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return vec4();
}

std::vector<vec4> DX11RenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	return std::vector<vec4>();
}

bool DX11RenderingServer::Resize()
{
	return true;
}

bool DX11RenderingServer::ReloadShader(RenderPassType renderPassType)
{
	return true;
}

bool DX11RenderingServer::BakeGIData()
{
	return true;
}