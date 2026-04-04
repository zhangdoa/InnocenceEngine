#include "DX12GraphicsResourceService.h"
#include "../IGraphicsService.h"

using namespace Inno;

MeshComponent* DX12GraphicsResourceService::AddMeshComponent(const char* name) { return m_Backend->AddMeshComponent(name); }
TextureComponent* DX12GraphicsResourceService::AddTextureComponent(const char* name) { return m_Backend->AddTextureComponent(name); }
MaterialComponent* DX12GraphicsResourceService::AddMaterialComponent(const char* name) { return m_Backend->AddMaterialComponent(name); }
RenderPassComponent* DX12GraphicsResourceService::AddRenderPassComponent(const char* name) { return m_Backend->AddRenderPassComponent(name); }
ShaderProgramComponent* DX12GraphicsResourceService::AddShaderProgramComponent(const char* name) { return m_Backend->AddShaderProgramComponent(name); }
SamplerComponent* DX12GraphicsResourceService::AddSamplerComponent(const char* name) { return m_Backend->AddSamplerComponent(name); }
GPUBufferComponent* DX12GraphicsResourceService::AddGPUBufferComponent(const char* name) { return m_Backend->AddGPUBufferComponent(name); }
CommandListComponent* DX12GraphicsResourceService::AddCommandListComponent(const char* name) { return m_Backend->AddCommandListComponent(name); }
IPipelineStateObject* DX12GraphicsResourceService::AddPipelineStateObject() { return m_Backend->AddPipelineStateObject(); }
ISemaphore* DX12GraphicsResourceService::AddSemaphore() { return m_Backend->AddSemaphore(); }
bool DX12GraphicsResourceService::Add(IOutputMergerTarget*& rhs) { return m_Backend->Add(rhs); }

bool DX12GraphicsResourceService::Delete(MeshComponent* mesh) { return m_Backend->Delete(mesh); }
bool DX12GraphicsResourceService::Delete(TextureComponent* texture) { return m_Backend->Delete(texture); }
bool DX12GraphicsResourceService::Delete(MaterialComponent* material) { return m_Backend->Delete(material); }
bool DX12GraphicsResourceService::Delete(RenderPassComponent* renderPass) { return m_Backend->Delete(renderPass); }
bool DX12GraphicsResourceService::Delete(ShaderProgramComponent* shaderProgram) { return m_Backend->Delete(shaderProgram); }
bool DX12GraphicsResourceService::Delete(SamplerComponent* sampler) { return m_Backend->Delete(sampler); }
bool DX12GraphicsResourceService::Delete(GPUBufferComponent* gpuBuffer) { return m_Backend->Delete(gpuBuffer); }
bool DX12GraphicsResourceService::Delete(IPipelineStateObject* rhs) { return m_Backend->Delete(rhs); }
bool DX12GraphicsResourceService::Delete(CommandListComponent* rhs) { return m_Backend->Delete(rhs); }
bool DX12GraphicsResourceService::Delete(ISemaphore* rhs) { return m_Backend->Delete(rhs); }
bool DX12GraphicsResourceService::Delete(IOutputMergerTarget* rhs) { return m_Backend->Delete(rhs); }

void DX12GraphicsResourceService::Initialize(EntityID entity) { m_Backend->Initialize(entity); }
void DX12GraphicsResourceService::Initialize(MeshComponent* mesh, std::vector<Vertex>& vertices, std::vector<Index>& indices, EntityID owner) { m_Backend->Initialize(mesh, vertices, indices, owner); }
void DX12GraphicsResourceService::Initialize(TextureComponent* texture, void* textureData, EntityID owner) { m_Backend->Initialize(texture, textureData, owner); }
void DX12GraphicsResourceService::Initialize(MaterialComponent* material, EntityID owner) { m_Backend->Initialize(material, owner); }
void DX12GraphicsResourceService::Initialize(RenderPassComponent* renderPass) { m_Backend->Initialize(renderPass); }
void DX12GraphicsResourceService::Initialize(ShaderProgramComponent* shaderProgram) { m_Backend->Initialize(shaderProgram); }
void DX12GraphicsResourceService::Initialize(SamplerComponent* sampler) { m_Backend->Initialize(sampler); }
void DX12GraphicsResourceService::Initialize(GPUBufferComponent* gpuBuffer) { m_Backend->Initialize(gpuBuffer); }
void DX12GraphicsResourceService::Initialize(CommandListComponent* commandList) { m_Backend->Initialize(commandList); }

bool DX12GraphicsResourceService::UploadToGPU(CommandListComponent* commandList, TextureComponent* texture) { return m_Backend->UploadToGPU(commandList, texture); }
bool DX12GraphicsResourceService::UploadToGPU(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return m_Backend->UploadToGPU(commandList, gpuBuffer); }
bool DX12GraphicsResourceService::Clear(CommandListComponent* commandList, TextureComponent* texture) { return m_Backend->Clear(commandList, texture); }
bool DX12GraphicsResourceService::Clear(CommandListComponent* commandList, GPUBufferComponent* gpuBuffer) { return m_Backend->Clear(commandList, gpuBuffer); }
bool DX12GraphicsResourceService::Copy(CommandListComponent* commandList, TextureComponent* src, TextureComponent* dst) { return m_Backend->Copy(commandList, src, dst); }
bool DX12GraphicsResourceService::GenerateMipmap(TextureComponent* texture, CommandListComponent* commandList) { return m_Backend->GenerateMipmap(texture, commandList); }

TextureComponent* DX12GraphicsResourceService::FindTextureByName(const char* name) { return m_Backend->FindTextureByName(name); }
MaterialComponent* DX12GraphicsResourceService::FindMaterialByName(const char* name) { return m_Backend->FindMaterialByName(name); }
std::optional<uint32_t> DX12GraphicsResourceService::GetIndex(TextureComponent* texture, Accessibility bindingAccessibility) { return m_Backend->GetIndex(texture, bindingAccessibility); }
Vec4 DX12GraphicsResourceService::ReadRenderTargetSample(RenderPassComponent* renderPass, size_t renderTargetIndex, size_t x, size_t y) { return m_Backend->ReadRenderTargetSample(renderPass, renderTargetIndex, x, y); }
std::vector<Vec4> DX12GraphicsResourceService::ReadTextureBackToCPU(RenderPassComponent* canvas, TextureComponent* textureComp) { return m_Backend->ReadTextureBackToCPU(canvas, textureComp); }

GPUMeshResource* DX12GraphicsResourceService::GetMeshResource(GPUMeshResourceHandle handle) { return m_Backend->GetMeshResource(handle); }
GPUMeshResourceHandle DX12GraphicsResourceService::FindMeshResourceByName(const char* name) { return m_Backend->FindMeshResourceByName(name); }

bool DX12GraphicsResourceService::WriteMappedMemory(GPUBufferComponent* gpuBuffer, IMappedMemory* mappedMemory, const void* sourceMemory, size_t startOffset, size_t range)
{
	return m_Backend->WriteMappedMemory(gpuBuffer, mappedMemory, sourceMemory, startOffset, range);
}

uint32_t DX12GraphicsResourceService::GetCurrentFrame() { return m_Backend->GetCurrentFrame(); }

GPUResourceComponent* DX12GraphicsResourceService::GetTLASBuffer() { return m_Backend->GetTLASBuffer(); }
