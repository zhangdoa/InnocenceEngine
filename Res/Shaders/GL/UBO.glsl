layout(std140, row_major, binding = 0) uniform cameraUBO
{
	mat4 uni_p_camera_original;
	mat4 uni_p_camera_jittered;
	mat4 uni_r_camera;
	mat4 uni_t_camera;
	mat4 uni_r_camera_prev;
	mat4 uni_t_camera_prev;
	vec4 uni_globalPos;
	float WHRatio;
	float zNear;
	float zFar;
};

layout(std140, row_major, binding = 1) uniform meshUBO
{
	mat4 uni_m;
	mat4 uni_m_prev;
	mat4 uni_normalMat;
	float uni_UUID;
};

layout(std140, row_major, binding = 2) uniform materialUBO
{
	vec4 uni_albedo;
	vec4 uni_MRAT;
	bool uni_useNormalTexture;
	bool uni_useAlbedoTexture;
	bool uni_useMetallicTexture;
	bool uni_useRoughnessTexture;
	bool uni_useAOTexture;
	int uni_materialType;
};

layout(std140, row_major, binding = 3) uniform sunUBO
{
	dirLight uni_dirLight;
};

layout(std140, row_major, binding = 4) uniform pointLightUBO
{
	pointLight uni_pointLights[NR_POINT_LIGHTS];
};

layout(std140, row_major, binding = 5) uniform sphereLightUBO
{
	sphereLight uni_sphereLights[NR_SPHERE_LIGHTS];
};

layout(std140, row_major, binding = 6) uniform CSMUBO
{
	CSM uni_CSMs[NR_CSM_SPLITS];
};

layout(std140, row_major, binding = 7) uniform skyUBO
{
	mat4 uni_p_inv;
	mat4 uni_v_inv;
	vec2 uni_viewportSize;
};

layout(std140, binding = 8) uniform dispatchParamsUBO
{
	uvec3 numThreadGroups;
	uint dispatchParamsUBO_padding1;
	uvec3 numThreads;
	uint dispatchParamsUBO_padding2;
};

layout(std430, binding = 0) buffer gridFrustumsSSBO
{
	Frustum gridFrustums[];
};

layout(std430, binding = 1) buffer lightListIndexCounterSSBO
{
	uint lightListIndexCounter;
};

layout(std430, binding = 2) buffer lightIndexListSSBO
{
	uint lightIndexList[];
};