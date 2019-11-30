#version 460
#define BLOCK_SIZE 16
#extension GL_ARB_shader_image_load_store : require
#extension GL_ARB_separate_shader_objects : enable
#extension GL_EXT_samplerless_texture_functions : require
//#define uni_drawCSMSplitedArea
//#define uni_drawPointLightShadow

struct PerFrame_CB
{
	mat4 p_original; // 0 - 3
	mat4 p_jittered; // 4 - 7
	mat4 v; // 8 - 11
	mat4 v_prev; // 12 - 15
	mat4 p_inv; // 16 - 19
	mat4 v_inv; // 20 - 23
	float zNear; // Tight packing 24
	float zFar; // Tight packing 24
	float minLogLuminance; // Tight packing 24
	float maxLogLuminance; // Tight packing 24
	vec4 sun_direction; // 25
	vec4 sun_illuminance; // 26
	vec4 viewportSize; // 27
	vec4 posWSNormalizer; // 28
	vec4 camera_posWS; // 29
	vec4 padding[2]; // 30 - 31
};

struct PerObject_CB
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	float UUID;
	float padding[3];
};

struct Material_CB
{
	vec4 albedo; // 0
	vec4 MRAT; // 1
	bool useNormalTexture;  // Tight packing 2
	bool useAlbedoTexture;  // Tight packing 2
	bool useMetallicTexture;  // Tight packing 2
	bool useRoughnessTexture;  // Tight packing 2
	bool useAOTexture; // Tight packing 3
	bool materialType; // Tight packing 3
	float padding[12]; // 4 - 15
};

// w component of luminousFlux is attenuationRadius
struct PointLight_CB
{
	vec4 position;
	vec4 luminousFlux;
	//float attenuationRadius;
};

// w component of luminousFlux is sphereRadius
struct SphereLight_CB
{
	vec4 position;
	vec4 luminousFlux;
	//float sphereRadius;
};

struct CSM_CB
{
	mat4 p;
	mat4 v;
	vec4 AABBMax;
	vec4 AABBMin;
	float padding[6];
};

struct DispatchParam_CB
{
	uvec4 numThreadGroups;
	uvec4 numThreads;
};

struct Plane
{
	vec3 N;
	float d;
};

struct Frustum
{
	Plane planes[4]; // LRTB
};

struct Sphere
{
	vec3 c;
	float r;
};

struct SH9
{
	vec4 L00;
	vec4 L11;
	vec4 L10;
	vec4 L1_1;
	vec4 L21;
	vec4 L2_1;
	vec4 L2_2;
	vec4 L20;
	vec4 L22;
};

struct Surfel
{
	vec4 pos;
	vec4 normal;
	vec4 albedo;
	vec4 MRAT;
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
	vec4 pos;
	uint brickFactorRange[12];
	float skyVisibility[6];
	uint padding[10];
};

const float eps = 0.00001;
const float PI = 3.14159265359;
const float SQRT_3 = 1.73205080f;
const int NR_POINT_LIGHTS = 1024;
const int NR_SPHERE_LIGHTS = 128;
const int NR_CSM_SPLITS = 4;

#include "common/CBuffers.glsl"
