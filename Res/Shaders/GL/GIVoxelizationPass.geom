// shadertype=glsl
#include "common.glsl"
layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) in VS_OUT
{
	vec2 texCoord;
	vec3 normal;
} gs_in[3];

layout(location = 0) out GS_OUT
{
	vec3 outputCoord;
	vec3 positionLS;
	vec3 normal;
	vec2 texCoord;
	flat vec4 triangleAABB;
} gs_out;

layout(location = 0) uniform mat4 uni_VP[3];
layout(location = 3) uniform mat4 uni_VP_inv[3];
layout(location = 6) uniform uint uni_volumeDimension;
layout(location = 7) uniform float uni_voxelScale;
layout(location = 8) uniform vec4 uni_worldMinPoint;

int CalculateAxis()
{
	vec3 p1 = gl_in[1].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 p2 = gl_in[2].gl_Position.xyz - gl_in[0].gl_Position.xyz;
	vec3 faceNormal = cross(p1, p2);

	float nDX = abs(faceNormal.x);
	float nDY = abs(faceNormal.y);
	float nDZ = abs(faceNormal.z);

	if (nDX > nDY && nDX > nDZ)
	{
		return 0;
	}
	else if (nDY > nDX && nDY > nDZ)
	{
		return 1;
	}
	else
	{
		return 2;
	}
}

vec4 AABB(vec4 pos[3], vec2 pixelDiagonal)
{
	vec4 aabb;

	aabb.xy = min(pos[2].xy, min(pos[1].xy, pos[0].xy));
	aabb.zw = max(pos[2].xy, max(pos[1].xy, pos[0].xy));

	// enlarge by half-pixel
	aabb.xy -= pixelDiagonal;
	aabb.zw += pixelDiagonal;

	return aabb;
}

void main()
{
	vec2 texCoord[3];
	int selectedIndex = CalculateAxis();
	mat4 VP = uni_VP[selectedIndex];
	mat4 VP_inv = uni_VP_inv[selectedIndex];

	for (int i = 0; i < gl_in.length(); i++)
	{
		texCoord[i] = gs_in[i].texCoord;
	}

	//transform vertices to clip space
	vec4 pos[3] = vec4[3]
	(
		VP * gl_in[0].gl_Position,
		VP * gl_in[1].gl_Position,
		VP * gl_in[2].gl_Position
		);

	for (int i = 0; i < pos.length(); i++)
	{
		pos[i] /= pos[i].w;
	}

	// xyz is normal, w is distance
	vec4 trianglePlane;
	trianglePlane.xyz = cross(pos[1].xyz - pos[0].xyz, pos[2].xyz - pos[0].xyz);
	trianglePlane.xyz = normalize(trianglePlane.xyz);
	trianglePlane.w = -dot(pos[0].xyz, trianglePlane.xyz);

	// change winding, otherwise there are artifacts for the back faces.
	if (dot(trianglePlane.xyz, vec3(0.0, 0.0, 1.0)) < 0.0)
	{
		vec4 vertexTemp = pos[2];
		vec2 texCoordTemp = texCoord[2];

		pos[2] = pos[1];
		texCoord[2] = texCoord[1];

		pos[1] = vertexTemp;
		texCoord[1] = texCoordTemp;
	}

	vec2 halfPixel = vec2(1.0f / uni_volumeDimension);

	if (trianglePlane.z == 0.0f) return;

	// expanded aabb for triangle
	gs_out.triangleAABB = AABB(pos, halfPixel);

	// calculate the plane through each edge of the triangle
	// in normal form for dilatation of the triangle
	vec3 planes[3];
	planes[0] = cross(pos[0].xyw - pos[2].xyw, pos[2].xyw);
	planes[1] = cross(pos[1].xyw - pos[0].xyw, pos[0].xyw);
	planes[2] = cross(pos[2].xyw - pos[1].xyw, pos[1].xyw);
	planes[0].z -= dot(halfPixel, abs(planes[0].xy));
	planes[1].z -= dot(halfPixel, abs(planes[1].xy));
	planes[2].z -= dot(halfPixel, abs(planes[2].xy));

	// calculate intersection between translated planes
	vec3 intersection[3];
	intersection[0] = cross(planes[0], planes[1]);
	intersection[1] = cross(planes[1], planes[2]);
	intersection[2] = cross(planes[2], planes[0]);
	intersection[0] /= intersection[0].z;
	intersection[1] /= intersection[1].z;
	intersection[2] /= intersection[2].z;

	// calculate dilated triangle vertices
	float z[3];
	z[0] = -(intersection[0].x * trianglePlane.x + intersection[0].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	z[1] = -(intersection[1].x * trianglePlane.x + intersection[1].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	z[2] = -(intersection[2].x * trianglePlane.x + intersection[2].y * trianglePlane.y + trianglePlane.w) / trianglePlane.z;
	pos[0].xyz = vec3(intersection[0].xy, z[0]);
	pos[1].xyz = vec3(intersection[1].xy, z[1]);
	pos[2].xyz = vec3(intersection[2].xy, z[2]);

	for (int i = 0; i < 3; ++i)
	{
		vec4 voxelPos = VP_inv * pos[i];
		voxelPos /= voxelPos.w;
		// To voxelized world space
		voxelPos.xyz -= uni_worldMinPoint.xyz;
		voxelPos *= uni_voxelScale;

		gl_Position = pos[i];
		gs_out.positionLS = pos[i].xyz;
		gs_out.normal = gs_in[i].normal;
		gs_out.texCoord = texCoord[i];
		gs_out.outputCoord = voxelPos.xyz * uni_volumeDimension;

		EmitVertex();
	}

	EndPrimitive();
}