#version 460
#define BLOCK_SIZE 16
#extension GL_ARB_shader_image_load_store : require
//#define uni_drawCSMSplitedArea
//#define uni_drawPointLightShadow

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

struct meshData
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	float UUID;
};

struct dirLight
{
	vec4 direction;
	vec4 illuminance;
	mat4 r;
};

// w component of luminousFlux is attenuationRadius
struct pointLight
{
	vec4 position;
	vec4 luminousFlux;
	//float attenuationRadius;
};

// w component of luminousFlux is sphereRadius
struct sphereLight
{
	vec4 position;
	vec4 luminousFlux;
	//float sphereRadius;
};

struct CSM {
	mat4 p;
	mat4 v;
	vec4 AABBMax;
	vec4 AABBMin;
	float padding[6];
};

struct DispatchParam
{
	uvec4 numThreadGroups;
	uvec4 numThreads;
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

#include "common/UBO.glsl"