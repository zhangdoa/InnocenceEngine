#pragma once
#include "../Common/InnoType.h"
#include "../Component/MeshDataComponent.h"
#include "../Component/MaterialDataComponent.h"
#include "../Component/TextureDataComponent.h"

struct alignas(16) CameraGPUData
{
	Mat4 p_original;
	Mat4 p_jittered;
	Mat4 r;
	Mat4 t;
	Mat4 r_prev;
	Mat4 t_prev;
	Vec4 globalPos;
	float WHRatio;
	float zNear;
	float zFar;
	float padding[25];
};

struct alignas(16) SunGPUData
{
	Vec4 dir;
	Vec4 luminance;
	Mat4 r;
	float padding[8];
};

struct alignas(16) CSMGPUData
{
	Mat4 p;
	Mat4 v;
	Vec4 AABBMax;
	Vec4 AABBMin;
	float padding[24];
};

// w component of luminance is attenuationRadius
struct alignas(16) PointLightGPUData
{
	Vec4 pos;
	Vec4 luminance;
	//float attenuationRadius;
};

// w component of luminance is sphereRadius
struct alignas(16) SphereLightGPUData
{
	Vec4 pos;
	Vec4 luminance;
	//float sphereRadius;
};

struct alignas(16) MeshGPUData
{
	Mat4 m;
	Mat4 m_prev;
	Mat4 normalMat;
	float UUID;
	float padding[15];
};

struct alignas(16) MaterialGPUData
{
	MeshCustomMaterial customMaterial;
	int32_t useNormalTexture = true;
	int32_t useAlbedoTexture = true;
	int32_t useMetallicTexture = true;
	int32_t useRoughnessTexture = true;
	int32_t useAOTexture = true;
	int32_t materialType = 0;
	float padding1[2];
	Mat4 padding2;
	Mat4 padding3;
	Mat4 padding4;
};

struct alignas(16) SkyGPUData
{
	Mat4 p_inv;
	Mat4 r_inv;
	Vec4 viewportSize;
	Vec4 posWSNormalizer;
	float padding[24];
};

struct alignas(16) DispatchParamsGPUData
{
	TVec4<uint32_t> numThreadGroups;
	TVec4<uint32_t> numThreads;
};

struct alignas(16) GICameraGPUData
{
	Mat4 p;
	Mat4 r[6];
	Mat4 t;
};

struct alignas(16) GISkyGPUData
{
	Mat4 p_inv;
	Mat4 v_inv[6];
	Vec4 probeCount;
	Vec4 probeInterval;
	Vec4 workload;
	Vec4 irradianceVolumeOffset;
};

struct OpaquePassDrawCallData
{
	MeshDataComponent* mesh;
	MaterialDataComponent* material;
};

struct TransparentPassDrawCallData
{
	MeshDataComponent* mesh;
	uint32_t meshGPUDataIndex;
	uint32_t materialGPUDataIndex;
};

struct BillboardPassDrawCallData
{
	TextureDataComponent* iconTexture;
	uint32_t meshGPUDataOffset;
	uint32_t instanceCount;
};

struct DebugPassDrawCallData
{
	MeshDataComponent* mesh;
};

// Sample point on geometry surface
struct Surfel
{
	Vec4 pos;
	Vec4 normal;
	Vec4 albedo;
	Vec4 MRAT;

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
	uint32_t surfelRangeBegin;
	uint32_t surfelRangeEnd;

	bool operator==(const Brick &other) const
	{
		return (boundBox.m_center == other.boundBox.m_center);
	}
};

struct BrickFactor
{
	float basisWeight;
	uint32_t brickIndex;

	bool operator==(const BrickFactor &other) const
	{
		return (brickIndex == other.brickIndex);
	}
};

struct Probe
{
	Vec4 pos;
	uint32_t brickFactorRange[12];
	float skyVisibility[6];
	uint32_t padding[10];
};

struct ProbeInfo
{
	Vec4 probeCount;
	Vec4 probeInterval;
};