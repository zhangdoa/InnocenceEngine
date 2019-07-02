#pragma once
#include "../Common/InnoType.h"
#include "../Component/MeshDataComponent.h"
#include "../Component/MaterialDataComponent.h"
#include "../Component/TextureDataComponent.h"

struct alignas(16) CameraGPUData
{
	mat4 p_original;
	mat4 p_jittered;
	mat4 r;
	mat4 t;
	mat4 r_prev;
	mat4 t_prev;
	vec4 globalPos;
	float WHRatio;
	float zNear;
	float zFar;
	float padding[25];
};

struct alignas(16) SunGPUData
{
	vec4 dir;
	vec4 luminance;
	mat4 r;
	float padding[32];
};

struct alignas(16) CSMGPUData
{
	mat4 p;
	mat4 v;
	vec4 AABBMax;
	vec4 AABBMin;
};

// w component of luminance is attenuationRadius
struct alignas(16) PointLightGPUData
{
	vec4 pos;
	vec4 luminance;
	//float attenuationRadius;
};

// w component of luminance is sphereRadius
struct alignas(16) SphereLightGPUData
{
	vec4 pos;
	vec4 luminance;
	//float sphereRadius;
};

struct alignas(16) MeshGPUData
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	float UUID;
	float padding[15];
};

struct alignas(16) MaterialGPUData
{
	MeshCustomMaterial customMaterial;
	int useNormalTexture = true;
	int useAlbedoTexture = true;
	int useMetallicTexture = true;
	int useRoughnessTexture = true;
	int useAOTexture = true;
	int materialType = 0;
	float padding1[2];
	mat4 padding2;
	mat4 padding3;
	mat4 padding4;
};

struct alignas(16) SkyGPUData
{
	mat4 p_inv;
	mat4 r_inv;
	vec2 viewportSize;
};

struct alignas(16) DispatchParamsGPUData
{
	TVec4<unsigned int> numThreadGroups;
	TVec4<unsigned int> numThreads;
};

struct OpaquePassGPUData
{
	MeshDataComponent* MDC;
	TextureDataComponent* normalTDC;
	TextureDataComponent* albedoTDC;
	TextureDataComponent* metallicTDC;
	TextureDataComponent* roughnessTDC;
	TextureDataComponent* AOTDC;
};

struct TransparentPassGPUData
{
	MeshDataComponent* MDC;
	unsigned int meshGPUDataIndex;
	unsigned int materialGPUDataIndex;
};

struct BillboardPassGPUData
{
	vec4 globalPos;
	float distanceToCamera;
	WorldEditorIconType iconType;
};

struct DebuggerPassGPUData
{
	mat4 m;
	MeshDataComponent* MDC;
};
