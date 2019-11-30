#version 450

struct ProbeMeshData
{
	mat4 m;
	vec4 index;
	vec4 padding[3];
};

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

layout(set = 1, binding = 0, std430) readonly buffer probeMeshSBuffer
{
	layout(row_major) ProbeMeshData _data[];
} probeMeshSBuffer_1;

layout(std140, row_major, set = 0, binding = 0) uniform perFrameCBufferBlock
{
	PerFrame_CB data;
} _58;

layout(location = 0) in vec4 input_position;
layout(location = 1) in vec2 input_texcoord;
layout(location = 2) in vec2 input_pada;
layout(location = 3) in vec4 input_normal;
layout(location = 4) in vec4 input_padb;
layout(location = 0) out vec4 _entryPointOutput_posWS;
layout(location = 1) out vec4 _entryPointOutput_probeIndex;
layout(location = 2) out vec4 _entryPointOutput_normal;

void main()
{
	vec4 _208 = probeMeshSBuffer_1._data[uint(gl_InstanceIndex)].m * input_position;
	gl_Position = _58.data.p_original * (_58.data.v * _208);
	_entryPointOutput_posWS = _208;
	_entryPointOutput_probeIndex = probeMeshSBuffer_1._data[uint(gl_InstanceIndex)].index;
	_entryPointOutput_normal = input_normal;
}
