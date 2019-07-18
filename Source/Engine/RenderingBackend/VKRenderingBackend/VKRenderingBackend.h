#pragma once
#include "../IRenderingBackend.h"

class VKRenderingBackend : INNO_IMPLEMENT IRenderingBackend
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(VKRenderingBackend);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool render() override;
	bool present() override;
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
	void registerUninitializedMaterialDataComponent(MaterialDataComponent* rhs) override;

	bool resize() override;
	bool reloadShader(RenderPassType renderPassType) override;
	bool bakeGI() override;
};