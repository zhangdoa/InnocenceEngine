#pragma once
#include "../../common/InnoType.h"

#include "../../component/MeshDataComponent.h"
#include "../../component/TextureDataComponent.h"
#include "../../component/VKMeshDataComponent.h"
#include "../../component/VKTextureDataComponent.h"
#include "../../component/VKShaderProgramComponent.h"
#include "../../component/VKRenderPassComponent.h"

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	bool initializeComponentPool();

	VKRenderPassComponent* addVKRenderPassComponent(unsigned int RTNum, TextureDataDesc RTDesc, const std::vector<VkImage>* VkImages, MeshPrimitiveTopology topology, VKShaderProgramComponent* VKSPC);
	bool destroyVKRenderPassComponent(VKRenderPassComponent* VKRPC);

	VKMeshDataComponent* generateVKMeshDataComponent(MeshDataComponent* rhs);
	VKTextureDataComponent* generateVKTextureDataComponent(TextureDataComponent* rhs);

	VKMeshDataComponent* addVKMeshDataComponent(EntityID rhs);
	VKTextureDataComponent* addVKTextureDataComponent(EntityID rhs);

	VKMeshDataComponent* getVKMeshDataComponent(EntityID rhs);
	VKTextureDataComponent* getVKTextureDataComponent(EntityID rhs);

	void recordDrawCall(VkCommandBuffer commandBuffer, EntityID rhs);
	void recordDrawCall(VkCommandBuffer commandBuffer, MeshDataComponent* MDC);
	void recordDrawCall(VkCommandBuffer commandBuffer, size_t indicesSize, VKMeshDataComponent * VKMDC);

	VKShaderProgramComponent* addVKShaderProgramComponent(const EntityID& rhs);

	bool initializeVKShaderProgramComponent(VKShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool activateVKShaderProgramComponent(VKShaderProgramComponent* rhs);
}