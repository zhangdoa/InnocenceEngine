#pragma once
#include "../common/InnoType.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/VKMeshDataComponent.h"
#include "../component/VKTextureDataComponent.h"
#include "../component/VKShaderProgramComponent.h"
#include "../component/VKRenderPassComponent.h"

INNO_PRIVATE_SCOPE VKRenderingSystemNS
{
	VKRenderPassComponent* addVKRenderPassComponent(unsigned int RTNum, TextureDataDesc RTDesc, VKShaderProgramComponent* VKSPC);

	VKMeshDataComponent* generateVKMeshDataComponent(MeshDataComponent* rhs);
	VKTextureDataComponent* generateVKTextureDataComponent(TextureDataComponent* rhs);

	VKMeshDataComponent* addVKMeshDataComponent(EntityID rhs);
	VKTextureDataComponent* addVKTextureDataComponent(EntityID rhs);

	VKMeshDataComponent* getVKMeshDataComponent(EntityID rhs);
	VKTextureDataComponent* getVKTextureDataComponent(EntityID rhs);

	void drawMesh(EntityID rhs);
	void drawMesh(MeshDataComponent* MDC);
	void drawMesh(size_t indicesSize, VKMeshDataComponent * VKMDC);

	bool initializeVKShaderProgramComponent(VKShaderProgramComponent* rhs, const ShaderFilePaths& shaderFilePaths);

	bool activateVKShaderProgramComponent(VKShaderProgramComponent* rhs);
}