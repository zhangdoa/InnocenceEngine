#pragma once
#include "IRenderingFrontendSystem.h"

class InnoRenderingFrontendSystem : INNO_IMPLEMENT IRenderingFrontendSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoRenderingFrontendSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;

	INNO_SYSTEM_EXPORT ObjectStatus getStatus() override;

	INNO_SYSTEM_EXPORT bool anyUninitializedMeshDataComponent() override;
	INNO_SYSTEM_EXPORT bool anyUninitializedTextureDataComponent() override;

	INNO_SYSTEM_EXPORT void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) override;
	INNO_SYSTEM_EXPORT void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) override;

	INNO_SYSTEM_EXPORT MeshDataComponent* acquireUninitializedMeshDataComponent() override;
	INNO_SYSTEM_EXPORT TextureDataComponent* acquireUninitializedTextureDataComponent() override;
};