// shadertype=glsl
#include "common/common.glsl"

layout(vertices = 16) out;

layout(location = 0) out int distanceRatio_out[];

const int distanceArray[6] = {
	32,
	16,
	8,
	4,
	2,
	1,
};

void main()
{	
	vec4 distanceA;
	vec4 distanceB;
	if(gl_InvocationID == gl_PatchVerticesIn * 4)
	{
		distanceA = gl_in[gl_InvocationID].gl_Position - cameraUBO.globalPos;
		distanceB = gl_in[gl_InvocationID - 1].gl_Position - cameraUBO.globalPos;
	}
	else
	{
		distanceA = gl_in[gl_InvocationID].gl_Position - cameraUBO.globalPos;
		distanceB = gl_in[gl_InvocationID + 1].gl_Position - cameraUBO.globalPos;
	}
	vec4 distanceAverange  = distanceA + distanceB;
	distanceAverange /= 2;

	float distance = length(distanceAverange);
	float drawDistance = cameraUBO.zFar - cameraUBO.zNear;
	float distanceRatio = distance / drawDistance;

	int distanceRatioI = int(distanceRatio * 5);
	distanceRatio_out[gl_InvocationID] = distanceRatioI;

	if(distanceRatioI > 5)
	{
		distanceRatioI = 5;
	}

	gl_TessLevelInner[0] = distanceArray[distanceRatioI];
	gl_TessLevelInner[1] = distanceArray[distanceRatioI];

	gl_TessLevelOuter[0] = distanceArray[distanceRatioI];
	gl_TessLevelOuter[1] = distanceArray[distanceRatioI];
	gl_TessLevelOuter[2] = distanceArray[distanceRatioI];
	gl_TessLevelOuter[3] = distanceArray[distanceRatioI];

	gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
}