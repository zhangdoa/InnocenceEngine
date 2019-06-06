#pragma once
#include "IRenderingFrontend.h"

INNO_CONCRETE InnoRenderingFrontend : INNO_IMPLEMENT IRenderingFrontend
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRenderingFrontend);

	bool setup(IRenderingBackend* renderingBackend) override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	MeshDataComponent* addMeshDataComponent() override;
	MaterialDataComponent* addMaterialDataComponent() override;
	TextureDataComponent* addTextureDataComponent() override;
	MeshDataComponent* getMeshDataComponent(MeshShapeType meshShapeType) override;
	TextureDataComponent* getTextureDataComponent(TextureUsageType textureUsageType) override;
	TextureDataComponent* getTextureDataComponent(FileExplorerIconType iconType) override;
	TextureDataComponent* getTextureDataComponent(WorldEditorIconType iconType) override;
	SkeletonDataComponent* addSkeletonDataComponent() override;
	AnimationDataComponent* addAnimationDataComponent() override;

	TVec2<unsigned int> getScreenResolution() override;
	bool setScreenResolution(TVec2<unsigned int> screenResolution) override;

	RenderingConfig getRenderingConfig() override;
	bool setRenderingConfig(RenderingConfig renderingConfig) override;
};