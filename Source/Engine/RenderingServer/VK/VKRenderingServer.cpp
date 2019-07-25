#include "VKRenderingServer.h"

bool VKRenderingServer::Setup()
{
	return false;
}

bool VKRenderingServer::Initialize()
{
	return false;
}

bool VKRenderingServer::Terminate()
{
	return false;
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

RenderPassComponent * VKRenderingServer::AddRenderPassComponent(const char * name)
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
	return false;
}

bool VKRenderingServer::InitializeTextureDataComponent(TextureDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::InitializeRenderPassComponent(RenderPassComponent * rhs)
{
	return false;
}

bool VKRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return false;
}

bool VKRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::DeleteMeshDataComponent(MeshDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::DeleteTextureDataComponent(TextureDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::DeleteMaterialDataComponent(MaterialDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::DeleteRenderPassComponent(RenderPassComponent * rhs)
{
	return false;
}

bool VKRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return false;
}

void VKRenderingServer::RegisterMeshDataComponent(MeshDataComponent * rhs)
{
}

void VKRenderingServer::RegisterMaterialDataComponent(MaterialDataComponent * rhs)
{
}

MeshDataComponent * VKRenderingServer::GetMeshDataComponent(MeshShapeType meshShapeType)
{
	return nullptr;
}

TextureDataComponent * VKRenderingServer::GetTextureDataComponent(TextureUsageType textureUsageType)
{
	return nullptr;
}

TextureDataComponent * VKRenderingServer::GetTextureDataComponent(WorldEditorIconType iconType)
{
	return nullptr;
}

bool VKRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent * rhs, const void * GPUBufferValue)
{
	return false;
}

bool VKRenderingServer::BindRenderPassComponent(RenderPassComponent * rhs)
{
	return false;
}

bool VKRenderingServer::CleanRenderTargets(RenderPassComponent * rhs)
{
	return false;
}

bool VKRenderingServer::BindGPUBuffer(ShaderType shaderType, GPUBufferAccessibility accessibility, GPUBufferDataComponent * GPUBufferDataComponent, size_t startOffset, size_t range)
{
	return false;
}

bool VKRenderingServer::BindShaderProgramComponent(ShaderProgramComponent * rhs)
{
	return false;
}

bool VKRenderingServer::BindMaterialDataComponent(MaterialDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::DispatchDrawCall(MeshDataComponent * rhs)
{
	return false;
}

bool VKRenderingServer::UnbindMaterialDataComponent(RenderPassComponent * rhs)
{
	return false;
}

bool VKRenderingServer::CommandListEnd(RenderPassComponent * rhs, size_t frameIndex)
{
	return false;
}

bool VKRenderingServer::ExecuteCommandList(RenderPassComponent * rhs, size_t frameIndex)
{
	return false;
}

bool VKRenderingServer::WaitForFrame(RenderPassComponent * rhs, size_t frameIndex)
{
	return false;
}

bool VKRenderingServer::Present()
{
	return false;
}

bool VKRenderingServer::CopyDepthBuffer(RenderPassComponent * src, RenderPassComponent * dest)
{
	return false;
}

bool VKRenderingServer::CopyStencilBuffer(RenderPassComponent * src, RenderPassComponent * dest)
{
	return false;
}

bool VKRenderingServer::CopyColorBuffer(RenderPassComponent * src, size_t srcIndex, RenderPassComponent * dest, size_t destIndex)
{
	return false;
}

vec4 VKRenderingServer::ReadRenderTargetSample(RenderPassComponent * rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return vec4();
}

std::vector<vec4> VKRenderingServer::ReadTextureBackToCPU(RenderPassComponent * canvas, TextureDataComponent * TDC)
{
	return std::vector<vec4>();
}

bool VKRenderingServer::Resize()
{
	return false;
}

bool VKRenderingServer::ReloadShader(RenderPassType renderPassType)
{
	return false;
}

bool VKRenderingServer::BakeGIData()
{
	return false;
}