layout(std140, row_major, set = 0, binding = 0) uniform perFrameCBufferBlock
{
	PerFrame_CB data;
} perFrameCBuffer;

layout(std140, row_major, set = 0, binding = 1) uniform perObjectCBufferBlock
{
	PerObject_CB data;
} perObjectCBuffer;

layout(std140, row_major, set = 0, binding = 2) uniform materialCBufferBlock
{
	Material_CB data;
} materialCBuffer;

layout(std140, row_major, set = 0, binding = 3) uniform pointLightCBufferBlock
{
	PointLight_CB data[NR_POINT_LIGHTS];
} pointLightCBuffer;

layout(std140, row_major, set = 0, binding = 4) uniform sphereLightCBufferBlock
{
	SphereLight_CB data[NR_SPHERE_LIGHTS];
} sphereLightCBuffer;

layout(std140, row_major, set = 0, binding = 5) uniform CSMCBufferBlock
{
	CSM_CB data[NR_CSM_SPLITS];
} CSMCBuffer;

layout(std140, set = 0, binding = 6) uniform dispatchParamsCBufferBlock
{
	DispatchParam_CB data[8];
} dispatchParamsCBuffer;

layout(std140, row_major, set = 0, binding = 7) uniform SSAOKernelCBufferBlock
{
	vec4 data[64];
} SSAOKernelCBuffer;

layout(std140, row_major, set = 0, binding = 8) uniform GICBufferBlock
{
	mat4 p;
	mat4 r[6];
	mat4 t;
	mat4 p_inv;
	mat4 v_inv[6];
	vec4 probeCount;
	vec4 probeRange;
	vec4 workload;
	vec4 irradianceVolumeOffset;
} GICBuffer;
