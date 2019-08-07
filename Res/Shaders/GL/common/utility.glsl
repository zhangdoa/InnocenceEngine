Plane ComputePlane(vec3 p0, vec3 p1, vec3 p2)
{
	Plane plane;

	vec3 v0 = p1 - p0;
	vec3 v2 = p2 - p0;

	plane.N = normalize(cross(v0, v2));
	plane.d = dot(plane.N, p0);

	return plane;
}

bool SphereInsidePlane(Sphere sphere, Plane plane)
{
	return dot(plane.N, sphere.c) - plane.d < -sphere.r;
}

bool SphereInsideFrustum(Sphere sphere, Frustum frustum, float zNear, float zFar)
{
	bool result = true;

	if (sphere.c.z - sphere.r > zNear || sphere.c.z + sphere.r < zFar)
	{
		result = false;
	}

	for (int i = 0; i < 4 && result; i++)
	{
		if (SphereInsidePlane(sphere, frustum.planes[i]))
		{
			result = false;
		}
	}

	return result;
}

vec4 ClipToView(vec4 clip, mat4 in_p_inv)
{
	// View space position.
	vec4 view = in_p_inv * clip;
	// Perspective projection.
	view /= view.w;

	return view;
}

vec4 ScreenToView(vec4 screen, vec2 in_viewportSize, mat4 in_p_inv)
{
	// Convert to normalized texture coordinates
	vec2 texCoord = screen.xy / in_viewportSize;

	// Convert to clip space
	vec4 clip = vec4(vec2(texCoord.x, texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

	return ClipToView(clip, in_p_inv);
}