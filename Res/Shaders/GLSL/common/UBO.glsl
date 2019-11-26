layout(std140, row_major, set = 0, binding = 0) uniform cameraUBOBlock
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

layout(std140, row_major, set = 0, binding = 1) uniform meshUBOBlock
{
	mat4 m;
	mat4 m_prev;
	mat4 normalMat;
	float UUID;
} meshUBO;

layout(std140, row_major, set = 0, binding = 2) uniform materialUBOBlock
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

layout(std140, row_major, set = 0, binding = 3) uniform sunUBOBlock
{
	dirLight data;
} sunUBO;

layout(std140, row_major, set = 0, binding = 4) uniform pointLightUBOBlock
{
	pointLight data[NR_POINT_LIGHTS];
} pointLightUBO;

layout(std140, row_major, set = 0, binding = 5) uniform sphereLightUBOBlock
{
	sphereLight data[NR_SPHERE_LIGHTS];
} sphereLightUBO;

layout(std140, row_major, set = 0, binding = 6) uniform CSMUBOBlock
{
	CSM data[NR_CSM_SPLITS];
} CSMUBO;

layout(std140, row_major, set = 0, binding = 7) uniform skyUBOBlock
{
	mat4 p_inv;
	mat4 v_inv;
	vec4 viewportSize;
	vec4 posWSNormalizer;
} skyUBO;

layout(std140, set = 0, binding = 8) uniform dispatchParamsUBOBlock
{
	DispatchParam data[8];
} dispatchParamsUBO;

layout(std140, row_major, set = 0, binding = 9) uniform SSAOKernelUBOBlock
{
	vec4 data[64];
} SSAOKernelUBO;

layout(std140, row_major, set = 0, binding = 10) uniform GICameraUBOBlock
{
	mat4 p;
	mat4 r[6];
	mat4 t;
} GICameraUBO;

layout(std140, row_major, set = 0, binding = 11) uniform GISkyUBOUBOBlock
{
	mat4 p_inv;
	mat4 v_inv[6];
	vec4 probeCount;
	vec4 probeRange;
	vec4 workload;
	vec4 irradianceVolumeOffset;
} GISkyUBO;

layout(std430, row_major, set = 0, binding = 12) buffer billboardUBOBlock
{
	meshData data[];
} billboardUBO;