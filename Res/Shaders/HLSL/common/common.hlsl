// shadertype=hlsl

static const float eps = 0.00001;
static const float PI = 3.14159265359;

static const int NR_POINT_LIGHTS = 1024;
static const int NR_SPHERE_LIGHTS = 128;
static const int NR_CSM_SPLITS = 4;

#define BLOCK_SIZE 16

struct PerFrame_CB
{
	matrix p_original; // 0 - 3
	matrix p_jittered; // 4 - 7
	matrix v; // 8 - 11
	matrix v_prev; // 12 - 15
	matrix p_inv; // 16 - 19
	matrix v_inv; // 20 - 23
	float zNear; // Tight packing 24
	float zFar; // Tight packing 24
	float minLogLuminance; // Tight packing 24
	float maxLogLuminance; // Tight packing 24
	float4 sun_direction; // 25
	float4 sun_illuminance; // 26
	float4 viewportSize; // 27
	float4 posWSNormalizer; // 28
	float4 camera_posWS; // 29
	float4 padding[2]; // 30 - 31
};

struct PerObject_CB
{
	matrix m;
	matrix m_prev;
	matrix normalMat;
	float UUID;
	float padding[3];
};

struct Material_CB
{
	float4 albedo; // 0
	float4 MRAT; // 1
	uint textureSlotMask; // 2
	uint materialType; // 2
	float padding3[13]; // 3 - 15
};

// w component of luminousFlux is attenuationRadius
struct PointLight_CB
{
	float4 position;
	float4 luminousFlux;
	//float attenuationRadius;
};

// w component of luminousFlux is sphereRadius
struct SphereLight_CB
{
	float4 position;
	float4 luminousFlux;
	//float sphereRadius;
};

struct SH9
{
	float4 L00;
	float4 L11;
	float4 L10;
	float4 L1_1;
	float4 L21;
	float4 L2_1;
	float4 L2_2;
	float4 L20;
	float4 L22;
};

struct CSM_CB
{
	matrix p;
	matrix v;
	float4 AABBMax;
	float4 AABBMin;
	float4 padding[6];
};

struct DispatchParam_CB
{
	uint4 numThreadGroups;
	uint4 numThreads;
};

struct GI_CB
{
	matrix p;
	matrix r[6];
	matrix t;
	matrix p_inv;
	matrix v_inv[6];
	float4 probeCount;
	float4 probeRange;
	float4 workload;
	float4 irradianceVolumeOffset;
};

struct VoxelizationPass_CB
{
	matrix VP[3];
	matrix VP_inv[3];
	float4 posWSOffset;
	float4 volumeSize;
	float4 voxelSize;
	float4 padding[5];
};

struct Plane
{
	float3 N;
	float  d;
};

Plane ComputePlane(float3 p0, float3 p1, float3 p2)
{
	Plane plane;

	float3 v0 = p1 - p0;
	float3 v2 = p2 - p0;

	plane.N = normalize(cross(v0, v2));
	plane.d = dot(plane.N, p0);

	return plane;
}

struct Frustum
{
	Plane planes[4]; // LRTB
};

struct Sphere
{
	float3 c;
	float  r;
};

bool SphereInsidePlane(Sphere sphere, Plane plane)
{
	return dot(plane.N, sphere.c) - plane.d < -sphere.r;
}

bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar)
{
	bool result = true;

	if (sphere.c.z - sphere.r > zNear || sphere.c.z + sphere.r < zFar)
	{
		result = false;
	}

	for (int i = 0; i < 4 && result; i++)
	{
		if (SphereInsidePlane(sphere, frustum.planes[i]))
		{
			result = false;
		}
	}

	return result;
}

float4 ClipToView(float4 clip, matrix in_p_inv)
{
	// View space position.
	float4 view = mul(clip, in_p_inv);
	// Perspective projection.
	view = view / view.w;

	return view;
}

float4 ScreenToView(float4 screen, float2 in_viewportSize, matrix in_p_inv)
{
	// Convert to normalized texture coordinates
	float2 texCoord = screen.xy / in_viewportSize;

	// Convert to clip space
	float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

	return ClipToView(clip, in_p_inv);
}

struct Surfel
{
	float4 pos;
	float4 normal;
	float4 albedo;
	float4 MRAT;
};

struct Brick
{
	uint surfelRangeBegin;
	uint surfelRangeEnd;
};

struct BrickFactor
{
	float basisWeight;
	uint brickIndex;
};

struct Probe
{
	float4 pos;
	uint brickFactorRange[12];
	float skyVisibility[6];
	uint padding[10];
};

#include "common/CBuffers.hlsl"