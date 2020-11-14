#include "MTRenderingServer.h"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

namespace MTRenderingServerNS
{
	MTRenderingServerBridge* m_bridge;
}

bool MTRenderingServer::Setup(ISystemConfig* systemConfig)
{
	return true;
}

bool MTRenderingServer::Initialize()
{
	return true;
}

bool MTRenderingServer::Terminate()
{
	return true;
}

ObjectStatus MTRenderingServer::GetStatus()
{
	return ObjectStatus();
}

MeshDataComponent* MTRenderingServer::AddMeshDataComponent(const char* name)
{
	return nullptr;
}

TextureDataComponent* MTRenderingServer::AddTextureDataComponent(const char* name)
{
	return nullptr;
}

MaterialDataComponent* MTRenderingServer::AddMaterialDataComponent(const char* name)
{
	return nullptr;
}

RenderPassDataComponent* MTRenderingServer::AddRenderPassDataComponent(const char* name)
{
	return nullptr;
}

ShaderProgramComponent* MTRenderingServer::AddShaderProgramComponent(const char* name)
{
	return nullptr;
}

SamplerDataComponent* MTRenderingServer::AddSamplerDataComponent(const char* name)
{
	return nullptr;
}

GPUBufferDataComponent* MTRenderingServer::AddGPUBufferDataComponent(const char* name)
{
	return nullptr;
}

bool MTRenderingServer::InitializeMeshDataComponent(MeshDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::InitializeTextureDataComponent(TextureDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::InitializeMaterialDataComponent(MaterialDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::InitializeRenderPassDataComponent(RenderPassDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::InitializeShaderProgramComponent(ShaderProgramComponent* rhs)
{
	return true;
}

bool MTRenderingServer::InitializeSamplerDataComponent(SamplerDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::InitializeGPUBufferDataComponent(GPUBufferDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::DeleteMeshDataComponent(MeshDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::DeleteTextureDataComponent(TextureDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::DeleteMaterialDataComponent(MaterialDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::DeleteRenderPassDataComponent(RenderPassDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::DeleteShaderProgramComponent(ShaderProgramComponent* rhs)
{
	return true;
}

bool MTRenderingServer::DeleteSamplerDataComponent(SamplerDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::DeleteGPUBufferDataComponent(GPUBufferDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::ClearTextureDataComponent(TextureDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::CopyTextureDataComponent(TextureDataComponent* lhs, TextureDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::UploadGPUBufferDataComponentImpl(GPUBufferDataComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	return true;
}

bool MTRenderingServer::ClearGPUBufferDataComponent(GPUBufferDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::CommandListBegin(RenderPassDataComponent* rhs, size_t frameIndex)
{
	return true;
}

bool MTRenderingServer::BindRenderPassDataComponent(RenderPassDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::CleanRenderTargets(RenderPassDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::ActivateResourceBinder(RenderPassDataComponent* renderPass, ShaderStage shaderStage, IResourceBinder* binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	return true;
}

bool MTRenderingServer::DrawIndexedInstanced(RenderPassDataComponent* renderPass, MeshDataComponent* mesh, size_t instanceCount)
{
	return true;
}

bool MTRenderingServer::DrawInstanced(RenderPassDataComponent* renderPass, size_t instanceCount)
{
	return true;
}

bool MTRenderingServer::DeactivateResourceBinder(RenderPassDataComponent* renderPass, ShaderStage shaderStage, IResourceBinder* binder, size_t globalSlot, size_t localSlot, Accessibility accessibility, size_t startOffset, size_t elementCount)
{
	return true;
}

bool MTRenderingServer::CommandListEnd(RenderPassDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::ExecuteCommandList(RenderPassDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::WaitForFrame(RenderPassDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::SetUserPipelineOutput(IResourceBinder* rhs)
{
	return true;
}

IResourceBinder* MTRenderingServer::GetUserPipelineOutput()
{
	return nullptr;
}

bool MTRenderingServer::Present()
{
	return true;
}

bool MTRenderingServer::Dispatch(RenderPassDataComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	return true;
}

Vec4 MTRenderingServer::ReadRenderTargetSample(RenderPassDataComponent* rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> MTRenderingServer::ReadTextureBackToCPU(RenderPassDataComponent* canvas, TextureDataComponent* TDC)
{
	return std::vector<Vec4>();
}

bool MTRenderingServer::GenerateMipmap(TextureDataComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Resize()
{
	return true;
}

void MTRenderingServer::setBridge(MTRenderingServerBridge* bridge)
{
	MTRenderingServerNS::m_bridge = bridge;
	g_Engine->getLogSystem()->Log(LogLevel::Success, "MTRenderingServer: Bridge connected at ", bridge);
}

bool MTRenderingServer::BeginCapture()
{
	return true;
}

bool MTRenderingServer::EndCapture()
{
	return true;
}