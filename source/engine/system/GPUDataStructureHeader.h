#include "../common/InnoType.h"
#include "../component/MeshDataComponent.h"
#include "../component/MaterialDataComponent.h"
#include "../component/TextureDataComponent.h"

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
	float padding[27];
};

struct alignas(16) SunGPUData
{
	vec4 dir;
	vec4 luminance;
	mat4 r;
};

struct alignas(16) CSMGPUData
{
	mat4 p;
	mat4 v;
	vec4 splitCorners;
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
};

struct GeometryPassGPUData
{
	MeshDataComponent* MDC;
	TextureDataComponent* normalTDC;
	TextureDataComponent* albedoTDC;
	TextureDataComponent* metallicTDC;
	TextureDataComponent* roughnessTDC;
	TextureDataComponent* AOTDC;
	MeshGPUData meshGPUData;
	MaterialGPUData materialGPUData;
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