// shadertype=glsl
#include "common/common.glsl"

layout(location = 0) out vec4 uni_debugPassRT0;

struct DebugMaterialData
{
	vec4 color;
};

layout(std430, row_major, set = 1, binding = 1) buffer debugMaterialSSBOBlock
{
	DebugMaterialData data[];
} debugMaterialSSBO;

layout(location = 0) in VS_OUT
{
	float materialID;
} fs_in;

void main()
{
	uni_debugPassRT0 = debugMaterialSSBO.data[int(fs_in.materialID)].color;
}