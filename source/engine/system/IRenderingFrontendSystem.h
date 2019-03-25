#pragma once
#include "../common/InnoType.h"
#include "../exports/InnoSystem_Export.h"
#include "../common/InnoClassTemplate.h"

#include "../component/MeshDataComponent.h"
#include "../component/TextureDataComponent.h"
#include "../component/MaterialDataComponent.h"

struct RenderingConfig
{
	int MSAAdepth = 4;
	bool useTAA = false;
	bool useBloom = false;
	bool drawTerrain = false;
	bool drawSky = false;
	bool drawDebugObject = false;
};

struct CameraDataPack
{
	mat4 p_Original;
	mat4 p_Jittered;
	mat4 r;
	mat4 t;
	mat4 r_prev;
	mat4 t_prev;
	vec4 globalPos;
	float WHRatio;
};

struct SunDataPack
{
	vec4 dir;
	vec4 luminance;
	mat4 r;
};

struct CSMDataPack
{
	mat4 p;
	mat4 v;
	vec4 splitCorners;
};

struct MeshDataPack
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	MeshDataComponent* MDC;
	MaterialDataComponent* material;
	VisiblilityType visiblilityType;
};

INNO_INTERFACE IRenderingFrontendSystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingFrontendSystem);

	INNO_SYSTEM_EXPORT virtual bool setup() = 0;
	INNO_SYSTEM_EXPORT virtual bool initialize() = 0;
	INNO_SYSTEM_EXPORT virtual bool update() = 0;
	INNO_SYSTEM_EXPORT virtual bool terminate() = 0;

	INNO_SYSTEM_EXPORT virtual ObjectStatus getStatus() = 0;

	INNO_SYSTEM_EXPORT virtual bool anyUninitializedMeshDataComponent() = 0;
	INNO_SYSTEM_EXPORT virtual bool anyUninitializedTextureDataComponent() = 0;

	INNO_SYSTEM_EXPORT virtual void registerUninitializedMeshDataComponent(MeshDataComponent* rhs) = 0;
	INNO_SYSTEM_EXPORT virtual void registerUninitializedTextureDataComponent(TextureDataComponent* rhs) = 0;

	INNO_SYSTEM_EXPORT virtual MeshDataComponent* acquireUninitializedMeshDataComponent() = 0;
	INNO_SYSTEM_EXPORT virtual TextureDataComponent* acquireUninitializedTextureDataComponent() = 0;

	INNO_SYSTEM_EXPORT virtual TVec2<unsigned int> getScreenResolution() = 0;
	INNO_SYSTEM_EXPORT virtual bool setScreenResolution(TVec2<unsigned int> screenResolution) = 0;

	INNO_SYSTEM_EXPORT virtual RenderingConfig getRenderingConfig() = 0;
	INNO_SYSTEM_EXPORT virtual bool setRenderingConfig(RenderingConfig renderingConfig) = 0;

	INNO_SYSTEM_EXPORT virtual CameraDataPack getCameraDataPack() = 0;
	INNO_SYSTEM_EXPORT virtual SunDataPack getSunDataPack() = 0;
	INNO_SYSTEM_EXPORT virtual std::vector<CSMDataPack>& getCSMDataPack() = 0;
	INNO_SYSTEM_EXPORT virtual std::optional<std::vector<MeshDataPack>> getMeshDataPack() = 0;

	INNO_SYSTEM_EXPORT virtual std::vector<Plane>& getDebugPlane() = 0;
	INNO_SYSTEM_EXPORT virtual std::vector<Sphere>& getDebugSphere() = 0;
};