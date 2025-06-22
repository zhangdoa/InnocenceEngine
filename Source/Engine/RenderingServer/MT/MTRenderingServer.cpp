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

bool MTRenderingServer::Initialize(MeshComponent* mesh)
{
	return true;
}

bool MTRenderingServer::Initialize(TextureComponent* texture)
{
	return true;
}

bool MTRenderingServer::Initialize(MaterialComponent* material)
{
	return true;
}

bool MTRenderingServer::Initialize(RenderPassComponent* renderPass)
{
	return true;
}

bool MTRenderingServer::Initialize(ShaderProgramComponent* shaderProgram)
{
	return true;
}

bool MTRenderingServer::Initialize(SamplerComponent* sampler)
{
	return true;
}

bool MTRenderingServer::Initialize(GPUBufferComponent* gpuBuffer)
{
	return true;
}

bool MTRenderingServer::Delete(MeshComponent* mesh)
{
	return true;
}

bool MTRenderingServer::Delete(TextureComponent* texture)
{
	return true;
}

bool MTRenderingServer::Delete(MaterialComponent* material)
{
	return true;
}

bool MTRenderingServer::Delete(RenderPassComponent* renderPass)
{
	return true;
}

bool MTRenderingServer::Delete(ShaderProgramComponent* shaderProgram)
{
	return true;
}

bool MTRenderingServer::Delete(SamplerComponent* sampler)
{
	return true;
}

bool MTRenderingServer::Delete(GPUBufferComponent* gpuBuffer)
{
	return true;
}

bool MTRenderingServer::Clear(TextureComponent* texture)
{
	return true;
}

bool MTRenderingServer::Copy(TextureComponent* sourceTexture, TextureComponent* texture)
{
	return true;
}

bool MTRenderingServer::UploadGPUBufferComponentImpl(GPUBufferComponent* gpuBuffer, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	return true;
}

bool MTRenderingServer::Clear(GPUBufferComponent* gpuBuffer)
{
	return true;
}

bool MTRenderingServer::CommandListBegin(CommandListComponent* commandList, RenderPassComponent* renderPass, size_t frameIndex)
{
	return true;
}

bool MTRenderingServer::BindRenderPassComponent(RenderPassComponent* renderPass)
{
	return true;
}

bool MTRenderingServer::ClearRenderTargets(RenderPassComponent* renderPass, size_t index)
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

bool MTRenderingServer::CommandListEnd(RenderPassComponent* renderPass)
{
	return true;
}

bool MTRenderingServer::Execute(RenderPassComponent* renderPass, GPUEngineType GPUEngineType)
{
	return true;
}

bool MTRenderingServer::WaitForFrame(RenderPassComponent* renderPass)
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

Vec4 MTRenderingServer::ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> MTRenderingServer::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
	return std::vector<Vec4>();
}

bool MTRenderingServer::GenerateMipmap(TextureComponent* texture)
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