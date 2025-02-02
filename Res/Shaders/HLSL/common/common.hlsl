// shadertype=hlsl

static const float eps = 0.00001;
static const float PI = 3.14159265359;
static const float SQRT2 = 1.41421356237;

static const int NR_POINT_LIGHTS = 1024;
static const int NR_SPHERE_LIGHTS = 128;
static const int NR_CSM_SPLITS = 4;

// @TODO: Do not hardcode this
static const int NUM_DISPATCH_PARAMS = 8;

static const float FLT_MIN = 1.175494351e-38;
static const float FLT_MAX = 3.402823466e+38;

static const float sunAngularRadius = 0.000071;

// Listing 45 [https://google.github.io/filament/Filament.md.html]
static const float4 debugColors[16] = {
	float4(0.0, 0.0, 0.0, 0.0),         // black
	float4(0.0, 0.0, 0.1647, 0.0),      // darkest blue
	float4(0.0, 0.0, 0.3647, 0.0),      // darker blue
	float4(0.0, 0.0, 0.6647, 0.0),      // dark blue
	float4(0.0, 0.0, 0.9647, 0.0),      // blue
	float4(0.0, 0.9255, 0.9255, 0.0),   // cyan
	float4(0.0, 0.5647, 0.0, 0.0),      // dark green
	float4(0.0, 0.7843, 0.0, 0.0),      // green
	float4(1.0, 1.0, 0.0, 0.0),         // yellow
	float4(0.90588, 0.75294, 0.0, 0.0), // yellow-orange
	float4(1.0, 0.5647, 0.0, 0.0),      // orange
	float4(1.0, 0.0, 0.0, 0.0),         // bright red
	float4(0.8392, 0.0, 0.0, 0.0),      // red
	float4(1.0, 0.0, 1.0, 0.0),         // magenta
	float4(0.6, 0.3333, 0.7882, 0.0),   // purple
	float4(1.0, 1.0, 1.0, 0.0)          // white
};

struct VertexInputType
{
	float3 posLS : POSITION;
	float3 normalLS : NORMAL;
	float3 tangentLS : TANGENT;
	float2 texCoord : TEXCOORD;
	float4 pad1 : PAD_A;
	uint instanceId : SV_InstanceID;
};

#define BLOCK_SIZE 16

struct PerFrame_CB
{
	float4x4 p_original; // 0 - 3
	float4x4 p_jittered; // 4 - 7
	float4x4 v; // 8 - 11
	float4x4 v_prev; // 12 - 15
	float4x4 p_inv; // 16 - 19
	float4x4 v_inv; // 20 - 23
	float zNear; // Tight packing 24
	float zFar; // Tight packing 24
	float minLogLuminance; // Tight packing 24
	float maxLogLuminance; // Tight packing 24
	float4 sun_direction; // 25
	float4 sun_illuminance; // 26
	float4 viewportSize; // 27
	float4 posWSNormalizer; // 28
	float4 camera_posWS; // 29
	float aperture; // Tight packing 30
	float shutterTime; // Tight packing 30
	float ISO; // Tight packing 30
	uint activeCascade; // Tight packing 30
	float4 padding; // 31
};

struct PerObject_CB
{
	float4x4 m; // 0 - 3
	float4x4 m_prev; // 4 - 7
	float4x4 normalMat; // 8 - 11
	float UUID; // Tight packing 12
	uint m_MaterialIndex; // Tight packing 12
	uint padding_a; // Tight packing 12
	uint padding_b; // Tight packing 12
	float4 padding_c[3]; // 13 - 15
};

static const uint MaxTextureSlotCount = 7;
struct Material_CB
{
	float4 albedo; // 0
	float4 MRAT; // 1
	uint m_TextureIndices_0; // Tight packing 2
	uint m_TextureIndices_1; // Tight packing 2
	uint m_TextureIndices_2; // Tight packing 2
	uint m_TextureIndices_3; // Tight packing 2
	uint m_TextureIndices_4; // Tight packing 3
	uint m_TextureIndices_5; // Tight packing 3
	uint m_TextureIndices_6; // Tight packing 3
	uint materialType; // Tight packing 3
};

// w component of luminousFlux is attenuationRadius
struct PointLight_CB
{
	float4 position;
	float4 luminousFlux;
	//float attenuationRadius;
};

// w component of luminousFlux is sphereRadius
struct SphereLight_CB
{
	float4 position;
	float4 luminousFlux;
	//float sphereRadius;
};

struct SH9
{
	float4 L00;
	float4 L11;
	float4 L10;
	float4 L1_1;
	float4 L21;
	float4 L2_1;
	float4 L2_2;
	float4 L20;
	float4 L22;
};

struct CSM_CB
{
	float4x4 p;
	float4x4 v;
	float4 AABBMax;
	float4 AABBMin;
	float4 padding[6];
};

struct DispatchParams_CB
{
	uint4 numThreadGroups;
	uint4 numThreads;
};

struct GI_CB
{
	float4x4 p;
	float4x4 r[6];
	float4x4 t;
	float4x4 p_inv;
	float4x4 v_inv[6];
	float4 probeCount;
	float4 probeRange;
	float4 workload;
	float4 irradianceVolumeOffset;
};

struct VoxelizationPass_CB
{
	float4 volumeCenter; // 0
	float volumeExtend; // Tight packing 1
	float volumeExtendRcp; // Tight packing 1
	float volumeResolution; // Tight packing 1
	float volumeResolutionRcp; // Tight packing 1
	float voxelSize; // Tight packing 2
	float voxelSizeRcp; // Tight packing 2
	float numCones; // Tight packing 2
	float numConesRcp; // Tight packing 2
	float coneTracingStep; // Tight packing 3
	float coneTracingMaxDistance; // Tight packing 3
};

struct AnimationPass_CB
{
	float4x4 rootOffsetfloat4x4; // 0-3
	float duration; // 4
	int numChannels; // 4
	int numTicks; // 4
	float currentTime; // 4
	float padding[11];  // 5 - 15
};

struct Plane
{
	float3 N;
	float d;
};

Plane ComputePlane(float3 p0, float3 p1, float3 p2)
{
	Plane plane;

	float3 v0 = p1 - p0;
	float3 v2 = p2 - p0;

	plane.N = normalize(cross(v0, v2));
	plane.d = dot(plane.N, p0);

	return plane;
}

struct Frustum
{
	Plane planes[4]; // Left, right, top, bottom
};

struct Sphere
{
	float3 c;
	float r;
};

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

float4 ClipToView(float4 clip, float4x4 in_p_inv)
{
	// View space position.
	float4 view = mul(clip, in_p_inv);
	// Perspective projection.
	view = view / view.w;

	return view;
}

float4 ScreenToView(float4 screen, float2 in_viewportSize, float4x4 in_p_inv)
{
	// Convert to normalized texture coordinates
	float2 texCoord = screen.xy / in_viewportSize;

	// Convert to clip space
	float4 clip = float4(float2(texCoord.x, 1.0f - texCoord.y) * 2.0f - 1.0f, screen.z, screen.w);

	return ClipToView(clip, in_p_inv);
}

uint EncodeColor(in float4 color)
{
	float HDR = length(color.rgb);
	color.rgb /= HDR;

	uint3 colorUInt = uint3(color.rgb * 255.0f);
	uint HDRUint = (uint)(saturate(HDR / 100000.0f) * 127.0f);
	uint colorMask = (HDRUint << 24u) | (colorUInt.r << 16u) | (colorUInt.g << 8u) | colorUInt.b;

	uint alpha = (color.a > 0 ? 1u : 0u);
	colorMask |= alpha << 31u;

	return colorMask;
}

float4 DecodeColor(in uint colorMask)
{
	float HDR;
	float4 color;

	HDR = (colorMask >> 24u) & 0x0000007f;
	color.r = (colorMask >> 16u) & 0x000000ff;
	color.g = (colorMask >> 8u) & 0x000000ff;
	color.b = colorMask & 0x000000ff;

	HDR /= 127.0f;
	color.rgb /= 255.0f;

	color.rgb *= HDR * 100000.0f;

	color.a = (colorMask >> 31u) & 0x00000001;

	return color;
}

uint EncodeNormal(in float4 normal)
{
	int3 iNormal = int3(normal.xyz * 255.0f);
	uint3 iNormalSigns;
	iNormalSigns.x = (iNormal.x >> 5) & 0x04000000;
	iNormalSigns.y = (iNormal.y >> 14) & 0x00020000;
	iNormalSigns.z = (iNormal.z >> 23) & 0x00000100;
	iNormal = abs(iNormal);
	uint normalMask = iNormalSigns.x | (iNormal.x << 18) | iNormalSigns.y | (iNormal.y << 9) | iNormalSigns.z | iNormal.z;
	return normalMask;
}

float4 DecodeNormal(in uint normalMask)
{
	int3 iNormal;
	iNormal.x = (normalMask >> 18) & 0x000000ff;
	iNormal.y = (normalMask >> 9) & 0x000000ff;
	iNormal.z = normalMask & 0x000000ff;
	int3 iNormalSigns;
	iNormalSigns.x = (normalMask >> 25) & 0x00000002;
	iNormalSigns.y = (normalMask >> 16) & 0x00000002;
	iNormalSigns.z = (normalMask >> 7) & 0x00000002;
	iNormalSigns = 1 - iNormalSigns;
	float3 normal = float3(iNormal) / 255.0f;
	normal *= iNormalSigns;
	return float4(normal, 1.0f);
}

float4 RGBA8ToFloat4(uint val)
{
    return float4(float((val & 0x000000FF)), 
    float((val & 0x0000FF00) >> 8U), 
    float((val & 0x00FF0000) >> 16U), 
    float((val & 0xFF000000) >> 24U));
}

uint Float4ToRGBA8(float4 val)
{
    return (uint(val.w) & 0x000000FF) << 24U | 
    (uint(val.z) & 0x000000FF) << 16U | 
    (uint(val.y) & 0x000000FF) << 8U | 
    (uint(val.x) & 0x000000FF);
}
float GetLuma(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

static const float3x3 RGB_XYZ_Factor = float3x3(
	0.4124564, 0.3575761, 0.1804375,
	0.2126729, 0.7151522, 0.0721750,
	0.0193339, 0.1191920, 0.9503041
	);

static const float3x3 XYZ_RGB_Factor = float3x3(
	3.2404542, -1.5371385, -0.4985314,
	-0.9692660, 1.8760108, 0.0415560,
	0.0556434, -0.2040259, 1.0572252
	);

float3 RGB_XYZ(float3 rgb)
{
	return mul(rgb, RGB_XYZ_Factor);
}

float3 XYZ_RGB(float3 xyz)
{
	return mul(xyz, XYZ_RGB_Factor);
}

float3 XYZ_XYY(float3 xyz)
{
	float Y = xyz.y;
	float x = xyz.x / (xyz.x + xyz.y + xyz.z);
	float y = xyz.y / (xyz.x + xyz.y + xyz.z);
	return float3(x, y, Y);
}

float3 XYY_XYZ(float3 xyY)
{
	float Y = xyY.z;
	float x = Y * xyY.x / xyY.y;
	float z = Y * (1.0 - xyY.x - xyY.y) / xyY.y;
	return float3(x, Y, z);
}

float3 RGB_XYY(float3 rgb)
{
	float3 xyz = RGB_XYZ(rgb);
	return XYZ_XYY(xyz);
}

float3 XYY_RGB(float3 xyY)
{
	float3 xyz = XYY_XYZ(xyY);
	return XYZ_RGB(xyz);
}

// [https://software.intel.com/en-us/node/503873]
float3 RGB_YCoCg(float3 c)
{
	// Y = R/4 + G/2 + B/4
	// Co = R/2 - B/2
	// Cg = -R/4 + G/2 - B/4
	return float3(
		c.x / 4.0 + c.y / 2.0 + c.z / 4.0,
		c.x / 2.0 - c.z / 2.0,
		-c.x / 4.0 + c.y / 2.0 - c.z / 4.0
		);
}

// [https://software.intel.com/en-us/node/503873]
float3 YCoCg_RGB(float3 c)
{
	// R = Y + Co - Cg
	// G = Y + Cg
	// B = Y - Co - Cg
	return float3(
		c.x + c.y - c.z,
		c.x + c.z,
		c.x - c.y - c.z
		);
}

float3 TonemapReinhard(float3 color)
{
	return color / (1.0f + color);
}

// [http://graphicrants.blogspot.com/2013/12/tone-mapping.html]
float3 TonemapReinhardLuma(float3 color)
{
	float luma = GetLuma(color);
	return color / (1.0f + luma);
}

// [http://graphicrants.blogspot.com/2013/12/tone-mapping.html]
float3 TonemapInvertReinhardLuma(float3 color)
{
	float luma = GetLuma(color);
	return color / (1.0f - luma);
}

// [https://gpuopen.com/learn/optimized-reversible-tonemapper-for-resolve/]
float3 TonemapMax3(float3 color)
{
	float maxValue = max(max(color.x, color.y), color.z);
	return color / (1.0f + maxValue);
}

// [https://gpuopen.com/learn/optimized-reversible-tonemapper-for-resolve/]
float3 TonemapInvertMax3(float3 color)
{
	float maxValue = max(max(color.x, color.y), color.z);
	return color / (1.0f - maxValue);
}

// Academy Color Encoding System
// [http://www.oscars.org/science-technology/sci-tech-projects/aces]
float3 TonemapACES(const float3 x)
{
	const float a = 2.51;
	const float b = 0.03;
	const float c = 2.43;
	const float d = 0.59;
	const float e = 0.14;
	return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

// gamma correction with respect to human eyes non-linearity
// [https://seblagarde.files.wordpress.com/2015/07/course_notes_moving_frostbite_to_pbr_v32.pdf]
float3 AccurateLinearToSRGB(float3 linearCol)
{
	float3 sRGBLo = linearCol * 12.92;
	float3 sRGBHi = (pow(abs(linearCol), 1.0 / 2.4) * 1.055) - 0.055;
	const float threshold = 0.0031308;
	const float3 threshold3 = float3(threshold, threshold, threshold);

	float sR = (linearCol.r <= threshold3.r) ? sRGBLo.r : sRGBHi.r;
	float sG = (linearCol.g <= threshold3.g) ? sRGBLo.g : sRGBHi.g;
	float sB = (linearCol.b <= threshold3.b) ? sRGBLo.b : sRGBHi.b;
	float3 sRGB = float3(sR, sG, sB);

	return sRGB;
}

float ComputeEV100(float aperture, float shutterTime, float ISO)
{
	return log2(aperture * aperture / shutterTime * 100 / ISO);
}

float ComputeEV100FromAvgLuminance(float avgLuminance)
{
	return log2(avgLuminance * 100.0f / 12.5f);
}
float ConvertEV100ToExposure(float EV100)
{
	float maxLuminance = 1.2f * pow(2.0f, EV100);
	return 1.0f / maxLuminance;
}

float3 HSVToRGB(float3 hsv)
{
    float c = hsv.z * hsv.y; // Chroma
    float h = hsv.x / 60.0f; // Hue segment
    float x = c * (1.0f - abs(fmod(h, 2.0f) - 1.0f)); // Intermediate value
    float3 rgb;

    if (h < 1.0f)
    {
        rgb = float3(c, x, 0.0f);
    }
    else if (h < 2.0f)
    {
        rgb = float3(x, c, 0.0f);
    }
    else if (h < 3.0f)
    {
        rgb = float3(0.0f, c, x);
    }
    else if (h < 4.0f)
    {
        rgb = float3(0.0f, x, c);
    }
    else if (h < 5.0f)
    {
        rgb = float3(x, 0.0f, c);
    }
    else
    {
        rgb = float3(c, 0.0f, x);
    }

    float m = hsv.z - c;
    rgb += m;

    return rgb;
}

struct Surfel
{
	float4 pos;
	float4 normal;
	float4 albedo;
	float4 MRAT;
};

struct Brick
{
	uint surfelRangeBegin;
	uint surfelRangeEnd;
};

struct BrickFactor
{
	float basisWeight;
	uint brickIndex;
};

struct Probe
{
	float4 pos;
	uint brickFactorRange[12];
	float skyVisibility[6];
	uint padding[10];
};