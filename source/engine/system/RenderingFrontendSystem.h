#pragma once
#include "IRenderingFrontendSystem.h"

INNO_CONCRETE InnoRenderingFrontendSystem : INNO_IMPLEMENT IRenderingFrontendSystem
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

	INNO_SYSTEM_EXPORT TVec2<unsigned int> getScreenResolution() override;
	INNO_SYSTEM_EXPORT bool setScreenResolution(TVec2<unsigned int> screenResolution) override;

	INNO_SYSTEM_EXPORT RenderingConfig getRenderingConfig() override;
	INNO_SYSTEM_EXPORT bool setRenderingConfig(RenderingConfig renderingConfig) override;

	INNO_SYSTEM_EXPORT std::optional<CameraDataPack> getCameraDataPack() override;
	INNO_SYSTEM_EXPORT std::optional<SunDataPack> getSunDataPack() override;
	INNO_SYSTEM_EXPORT std::optional<std::vector<CSMDataPack>> getCSMDataPack() override;
	INNO_SYSTEM_EXPORT std::optional<std::vector<MeshDataPack>> getMeshDataPack() override;

	INNO_SYSTEM_EXPORT std::vector<Plane>& getDebugPlane() override;
	INNO_SYSTEM_EXPORT std::vector<Sphere>& getDebugSphere() override;
};