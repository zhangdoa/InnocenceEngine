#pragma once
#include "../IRenderingBackendSystem.h"

class GLRenderingSystem : INNO_IMPLEMENT IRenderingBackendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(GLRenderingSystem);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool render() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	MeshDataComponent* addMeshDataComponent() override;
	MaterialDataComponent* addMaterialDataComponent() override;
	TextureDataComponent* addTextureDataComponent() override;
	MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) override;
	TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) override;
	TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) override;
	TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;

	void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) override;
	void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) override;

	bool resize() override;
	bool reloadShader(RenderPassType renderPassType) override;
	bool bakeGI() override;
};