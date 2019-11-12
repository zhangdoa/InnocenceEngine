#version 450

struct ProbeMeshData
{
	mat4 m;
	vec4 index;
	vec4 padding[3];
};

struct CameraData
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
	float padding[6];
};

layout(set = 1, binding = 0, std430) readonly buffer probeMeshSBuffer
{
	layout(row_major) ProbeMeshData _data[];
} probeMeshSBuffer_1;

layout(set = 0, binding = 0, std140) uniform cameraCBuffer
{
	layout(row_major) CameraData cameraCBuffer;
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
	gl_Position = _58.cameraCBuffer.p_original * (_58.cameraCBuffer.r * (_58.cameraCBuffer.t * _208));
	_entryPointOutput_posWS = _208;
	_entryPointOutput_probeIndex = probeMeshSBuffer_1._data[uint(gl_InstanceIndex)].index;
	_entryPointOutput_normal = input_normal;
}