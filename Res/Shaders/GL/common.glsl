#version 450
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

struct dirLight {
	vec4 direction;
	vec4 luminance;
	mat4 r;
};

// w component of luminance is attenuationRadius
struct pointLight {
	vec4 position;
	vec4 luminance;
	//float attenuationRadius;
};

// w component of luminance is sphereRadius
struct sphereLight {
	vec4 position;
	vec4 luminance;
	//float sphereRadius;
};

struct CSM {
	mat4 p;
	mat4 v;
	vec4 AABBMax;
	vec4 AABBMin;
};

const float eps = 0.00001;
const float PI = 3.14159265359;
const float SQRT_3 = 1.73205080f;
const int NR_POINT_LIGHTS = 1024;
const int NR_SPHERE_LIGHTS = 128;
const int NR_CSM_SPLITS = 4;

#include "UBO.glsl"