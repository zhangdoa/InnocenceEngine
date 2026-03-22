#include "MTGraphicsService.h"

#include "../../Engine.h"

using namespace Inno;
;

namespace MTGraphicsServiceNS
{
	MTGraphicsServiceBridge* m_bridge;
}

bool MTGraphicsService::Setup(IServiceConfig* systemConfig)
{
	return true;
}

bool MTGraphicsService::Initialize()
{
	return true;
}

bool MTGraphicsService::Terminate()
{
	return true;
}

ObjectStatus MTGraphicsService::GetStatus()
{
	return ObjectStatus();
}

bool MTGraphicsService::Initialize(MeshComponent* mesh)
{
	return true;
}

bool MTGraphicsService::Initialize(TextureComponent* texture)
{
	return true;
}

bool MTGraphicsService::Initialize(MaterialComponent* material)
{
	return true;
}

bool MTGraphicsService::Initialize(RenderPassComponent* renderPass)
{
	return true;
}

bool MTGraphicsService::Initialize(ShaderProgramComponent* shaderProgram)
{
	return true;
}

bool MTGraphicsService::Initialize(SamplerComponent* sampler)
{
	return true;
}

bool MTGraphicsService::Initialize(GPUBufferComponent* gpuBuffer)
{
	return true;
}

bool MTGraphicsService::Delete(MeshComponent* mesh)
{
	return true;
}

bool MTGraphicsService::Delete(TextureComponent* texture)
{
	return true;
}

bool MTGraphicsService::Delete(MaterialComponent* material)
{
	return true;
}

bool MTGraphicsService::Delete(RenderPassComponent* renderPass)
{
	return true;
}

bool MTGraphicsService::Delete(ShaderProgramComponent* shaderProgram)
{
	return true;
}

bool MTGraphicsService::Delete(SamplerComponent* sampler)
{
	return true;
}

bool MTGraphicsService::Delete(GPUBufferComponent* gpuBuffer)
{
	return true;
}

bool MTGraphicsService::Clear(TextureComponent* texture)
{
	return true;
}

bool MTGraphicsService::Copy(TextureComponent* sourceTexture, TextureComponent* texture)
{
	return true;
}

bool MTGraphicsService::UploadGPUBufferComponentImpl(GPUBufferComponent* gpuBuffer, const void* GPUBufferValue, size_t startOffset, size_t range)
{
	return true;
}

bool MTGraphicsService::Clear(GPUBufferComponent* gpuBuffer)
{
	return true;
}

bool MTGraphicsService::CommandListBegin(CommandListComponent* commandList, RenderPassComponent* renderPass, size_t frameIndex)
{
	return true;
}

bool MTGraphicsService::BindRenderPassComponent(RenderPassComponent* renderPass)
{
	return true;
}

bool MTGraphicsService::ClearRenderTargets(RenderPassComponent* renderPass, size_t index)
{
	return true;
}

bool MTGraphicsService::BindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return true;
}

bool MTGraphicsService::DrawIndexedInstanced(RenderPassComponent* renderPass, MeshComponent* mesh, size_t instanceCount)
{
	return true;
}

bool MTGraphicsService::DrawInstanced(RenderPassComponent* renderPass, size_t instanceCount)
{
	return true;
}

bool MTGraphicsService::UnbindGPUResource(RenderPassComponent* renderPass, ShaderStage shaderStage, GPUResourceComponent* resource, size_t resourceBindingLayoutDescIndex, size_t startOffset, size_t elementCount)
{
	return true;
}

bool MTGraphicsService::CommandListEnd(RenderPassComponent* renderPass)
{
	return true;
}

bool MTGraphicsService::Execute(RenderPassComponent* renderPass, GPUEngineType GPUEngineType)
{
	return true;
}

bool MTGraphicsService::WaitForFrame(RenderPassComponent* renderPass)
{
	return true;
}

bool MTGraphicsService::SetUserPipelineOutput(std::function<GPUResourceComponent*()>&& getUserPipelineOutputFunc)
{
	return true;
}

GPUResourceComponent* MTGraphicsService::GetUserPipelineOutput()
{
	return nullptr;
}

bool MTGraphicsService::Present()
{
	return true;
}

bool MTGraphicsService::Dispatch(RenderPassComponent* renderPass, uint32_t threadGroupX, uint32_t threadGroupY, uint32_t threadGroupZ)
{
	return true;
}

Vec4 MTGraphicsService::ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y)
{
	return Vec4();
}

std::vector<Vec4> MTGraphicsService::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* TextureComp)
{
	return std::vector<Vec4>();
}

bool MTGraphicsService::GenerateMipmap(TextureComponent* texture)
{
	return true;
}

bool MTGraphicsService::Resize()
{
	return true;
}

void MTGraphicsService::setBridge(MTGraphicsServiceBridge* bridge)
{
	MTGraphicsServiceNS::m_bridge = bridge;
	Log(Success, "Bridge connected at ", bridge);
}

bool MTGraphicsService::BeginCapture()
{
	return true;
}

bool MTGraphicsService::EndCapture()
{
	return true;
}