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
	float padding[8];
};

struct alignas(16) CSMGPUData
{
	mat4 p;
	mat4 v;
	vec4 AABBMax;
	vec4 AABBMin;
	float padding[24];
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
	vec4 posWSNormalizer;
	vec2 viewportSize;
	float padding[26];
};

struct alignas(16) DispatchParamsGPUData
{
	TVec4<unsigned int> numThreadGroups;
	TVec4<unsigned int> numThreads;
};

struct OpaquePassDrawCallData
{
	MeshDataComponent* mesh;
	MaterialDataComponent* material;
};

struct TransparentPassDrawCallData
{
	MeshDataComponent* mesh;
	unsigned int meshGPUDataIndex;
	unsigned int materialGPUDataIndex;
};

struct BillboardPassDrawCallData
{
	TextureDataComponent* iconTexture;
	unsigned int meshGPUDataOffset;
	unsigned int instanceCount;
};

struct DebugPassDrawCallData
{
	MeshDataComponent* mesh;
};

// Sample point on geometry surface
struct Surfel
{
	vec4 pos;
	vec4 normal;
	vec4 albedo;
	vec4 MRAT;

	bool operator==(const Surfel &other) const
	{
		return (pos == other.pos);
	}
};

// 1x1x1 m^3 of surfels
using SurfelGrid = Surfel;

// 4x4x4 m^3 of surfels
struct Brick
{
	AABB boundBox;
	unsigned int surfelRangeBegin;
	unsigned int surfelRangeEnd;

	bool operator==(const Brick &other) const
	{
		return (boundBox.m_center == other.boundBox.m_center);
	}
};

struct BrickFactor
{
	float basisWeight;
	unsigned int brickIndex;

	bool operator==(const BrickFactor &other) const
	{
		return (brickIndex == other.brickIndex);
	}
};

struct Probe
{
	vec4 pos;
	unsigned int brickFactorRange[12];
	float skyVisibility[6];
	unsigned int padding[10];
};