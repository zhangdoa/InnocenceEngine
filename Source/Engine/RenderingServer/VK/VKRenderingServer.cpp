#include "VKRenderingServer.h"

bool VKRenderingServer::Setup()
{
	return true;
}

bool VKRenderingServer::Initialize()
{
	return true;
}

bool VKRenderingServer::Terminate()
{
	return true;
}

ObjectStatus VKRenderingServer::GetStatus()
{
	return ObjectStatus();
}

MeshDataComponent * VKRenderingServer::AddMeshDataComponent(const char * name)
{
	return nullptr;
}

TextureDataComponent * VKRenderingServer::AddTextureDataComponent(const char * name)
{
	return nullptr;
}

MaterialDataComponent * VKRenderingServer::AddMaterialDataComponent(const char * name)
{
	return nullptr;
}

RenderPassDataComponent * VKRenderingServer::AddRenderPassDataComponent(const char * name)
{
	return nullptr;
}

ShaderProgramComponent * VKRenderingServer::AddShaderProgramComponent(const char * name)
{
	return nullptr;
}

GPUBufferDataComponent * VKRenderingServer::AddGPUBufferDataComponent(const char * name)
{
	return nullptr;
}

bool VKRenderingServer::InitializeMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool VKRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue)
{
	return true;
}

bool VKRenderingServer::CommandListBegin(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool VKRenderingServer::BindRenderPassDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::CleanRenderTargets(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::BindGPUBuffer(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent * GPUBufferDataComponent, size_t startOffset, size_t range)
{
	return true;
}

bool VKRenderingServer::BindShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return true;
}

bool VKRenderingServer::BindMaterialDataComponent(MaterialDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::DispatchDrawCall(MeshDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::UnbindMaterialDataComponent(RenderPassDataComponent * rhs)
{
	return true;
}

bool VKRenderingServer::CommandListEnd(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool VKRenderingServer::ExecuteCommandList(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool VKRenderingServer::WaitForFrame(RenderPassDataComponent * rhs, size_t frameIndex)
{
	return true;
}

bool VKRenderingServer::Present()
{
	return true;
}

bool VKRenderingServer::CopyDepthBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool VKRenderingServer::CopyStencilBuffer(RenderPassDataComponent * src, RenderPassDataComponent * dest)
{
	return true;
}

bool VKRenderingServer::CopyColorBuffer(RenderPassDataComponent * src, size_t srcIndex, RenderPassDataComponent * dest, size_t destIndex)
{
	return true;
}

vec4 VKRenderingServer::ReadRenderTargetSample(RenderPassDataComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return vec4();
}

std::vector<vec4> VKRenderingServer::ReadTextureBackToCPU(RenderPassDataComponent * canvas, TextureDataComponent * TDC)
{
	return std::vector<vec4>();
}

bool VKRenderingServer::Resize()
{
	return true;
}

bool VKRenderingServer::ReloadShader(RenderPassType renderPassType)
{
	return true;
}

bool VKRenderingServer::BakeGIData()
{
	return true;
}