layout(std140, row_major, binding = 0) uniform cameraUBOBlock
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
} cameraUBO;

layout(std140, row_major, binding = 1) uniform meshUBOBlock
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	float UUID;
} meshUBO;

layout(std140, row_major, binding = 2) uniform materialUBOBlock
{
	vec4 Albedo;
	vec4 MRAT;
	bool useNormalTexture;
	bool useAlbedoTexture;
	bool useMetallicTexture;
	bool useRoughnessTexture;
	bool useAOTexture;
	int materialType;
} materialUBO;

layout(std140, row_major, binding = 3) uniform sunUBOBlock
{
	dirLight data;
} sunUBO;

layout(std140, row_major, binding = 4) uniform pointLightUBOBlock
{
	pointLight data[NR_POINT_LIGHTS];
} pointLightUBO;

layout(std140, row_major, binding = 5) uniform sphereLightUBOBlock
{
	sphereLight data[NR_SPHERE_LIGHTS];
} sphereLightUBO;

layout(std140, row_major, binding = 6) uniform CSMUBOBlock
{
	CSM data[NR_CSM_SPLITS];
} CSMUBO;

layout(std140, row_major, binding = 7) uniform skyUBOBlock
{
	mat4 p_inv;
	mat4 v_inv;
	vec2 viewportSize;
} skyUBO;

layout(std140, binding = 8) uniform dispatchParamsUBOBlock
{
	uvec3 numThreadGroups;
	uint padding1;
	uvec3 numThreads;
	uint padding2;
} dispatchParamsUBO;

layout(std140, row_major, binding = 9) uniform SH9UBOBlock
{
	SH9 data[64];
} SH9UBO;

layout(std430, binding = 0) buffer gridFrustumsSSBOBlock
{
	Frustum data[];
} gridFrustumsSSBO;

layout(std430, binding = 1) buffer lightListIndexCounterSSBOBlock
{
	uint data;
} lightListIndexCounterSSBO;

layout(std430, binding = 2) buffer lightIndexListSSBOBlock
{
	uint data[];
} lightIndexListSSBO;