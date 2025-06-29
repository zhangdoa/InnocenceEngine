#include "MTRenderingServer.h"

#include "../../Engine.h"

using namespace Inno;
;

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

MeshComponent* MTRenderingServer::AddMeshComponent(const char* name)
{
	return nullptr;
}

TextureComponent* MTRenderingServer::AddTextureComponent(const char* name)
{
	return nullptr;
}

MaterialComponent* MTRenderingServer::AddMaterialComponent(const char* name)
{
	return nullptr;
}

RenderPassComponent* MTRenderingServer::AddRenderPassComponent(const char* name)
{
	return nullptr;
}

ShaderProgramComponent* MTRenderingServer::AddShaderProgramComponent(const char* name)
{
	return nullptr;
}

SamplerComponent* MTRenderingServer::AddSamplerComponent(const char* name)
{
	return nullptr;
}

GPUBufferComponent* MTRenderingServer::AddGPUBufferComponent(const char* name)
{
	return nullptr;
}

bool MTRenderingServer::Initialize(MeshComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Initialize(TextureComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Initialize(MaterialComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Initialize(RenderPassComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Initialize(ShaderProgramComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Initialize(SamplerComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Initialize(GPUBufferComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Delete(MeshComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Delete(TextureComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Delete(MaterialComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Delete(RenderPassComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Delete(ShaderProgramComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Delete(SamplerComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Delete(GPUBufferComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Clear(TextureComponent* rhs)
{
	return true;
}

bool MTRenderingServer::Copy(TextureComponent* lhs, TextureComponent* rhs)
{
	return true;
}

bool MTRenderingServer::UploadGPUBufferComponentImpl(GPUBufferComponent* rhs, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	return true;
}

bool MTRenderingServer::Clear(GPUBufferComponent* rhs)
{
	return true;
}

bool MTRenderingServer::CommandListBegin(RenderPassComponent* rhs, size_t frameIndex)
{
	return true;
}

bool MTRenderingServer::BindRenderPassComponent(RenderPassComponent* rhs)
{
	return true;
}

bool MTRenderingServer::ClearRenderTargets(RenderPassComponent* rhs, size_t index)
{
	return true;
}

bool MTRenderingServer::BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return true;
}

bool MTRenderingServer::DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount)
{
	return true;
}

bool MTRenderingServer::DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount)
{
	return true;
}

bool MTRenderingServer::UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return true;
}

bool MTRenderingServer::CommandListEnd(RenderPassComponent* rhs)
{
	return true;
}

bool MTRenderingServer::ExecuteCommandList(RenderPassComponent* rhs, GPUEngineType GPUEngineType)
{
	return true;
}

bool MTRenderingServer::WaitForFrame(RenderPassComponent* rhs)
{
	return true;
}

bool MTRenderingServer::SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc)
{
	return true;
}

GPUResourceComponent* MTRenderingServer::GetUserPipelineOutput()
{
	return nullptr;
}

bool MTRenderingServer::Present()
{
	return true;
}

bool MTRenderingServer::Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	return true;
}

Vec4 MTRenderingServer::ReadRenderTargetSample(RenderPassComponent* rhs, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> MTRenderingServer::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
	return std::vector<Vec4>();
}

bool MTRenderingServer::GenerateMipmap(TextureComponent* rhs)
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
	Log(Success, "Bridge connected at ", bridge);
}

bool MTRenderingServer::BeginCapture()
{
	return true;
}

bool MTRenderingServer::EndCapture()
{
	return true;
}