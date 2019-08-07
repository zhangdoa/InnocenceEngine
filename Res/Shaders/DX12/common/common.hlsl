// shadertype=hlsl

static const float eps = 0.00001;
static const float PI = 3.14159265359;

static const int NR_POINT_LIGHTS = 1024;
static const int NR_SPHERE_LIGHTS = 128;
static const int NR_CSM_SPLITS = 4;

#define BLOCK_SIZE 16

struct cameraData
{
	matrix p_original;
	matrix p_jittered;
	matrix r;
	matrix t;
	matrix r_prev;
	matrix t_prev;
	float4 globalPos;
	float WHRatio;
	float zNear;
	float zFar;
	float padding[25];
};

struct meshData
{
	matrix m;
	matrix m_prev;
	matrix normalMat;
	float UUID;
	float padding[15];
};

struct materialData
{
	float4 albedo;
	float4 MRAT;
	bool useNormalTexture;
	bool useAlbedoTexture;
	bool useMetallicTexture;
	bool useRoughnessTexture;
	bool useAOTexture;
	bool padding[3];
};

// w component of luminance is attenuationRadius
struct pointLight
{
	float4 position;
	float4 luminance;
	//float attenuationRadius;
};

// w component of luminance is sphereRadius
struct sphereLight
{
	float4 position;
	float4 luminance;
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

struct CSM {
	matrix p;
	matrix v;
	float4 AABBMax;
	float4 AABBMin;
	float4 padding[6];
};

struct DispatchParam {
	uint4 numThreadGroups;
	uint4 numThreads;
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

#include "common/GPUBuffers.hlsl"