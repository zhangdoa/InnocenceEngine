#pragma once
#include "../../Common/InnoType.h"
#include "../../Common/InnoMath.h"
#include "../../Component/GLRenderPassComponent.h"

// Sample point on geometry surface
struct Surfel
{
	vec4 pos;
	vec4 normal;
	vec4 albedo;
	vec4 MRAT;

	bool operator==(const Surfel &other) const
	{
		return (pos == other.pos);
	}
};

// 1x1x1 m^3 of surfels
using SurfelGrid = Surfel;

// 4x4x4 m^3 of surfels
struct Brick
{
	AABB boundBox;
	uint32_t surfelRangeBegin;
	uint32_t surfelRangeEnd;

	bool operator==(const Brick &other) const
	{
		return (boundBox.m_center == other.boundBox.m_center);
	}
};

struct BrickFactor
{
	float basisWeight;
	uint32_t brickIndex;
};

struct Probe
{
	vec4 pos;
	SH9 skyVisibility;
	SH9 radiance;
	uint32_t brickFactorRangeBegin;
	uint32_t brickFactorRangeEnd;
};

namespace GLEnvironmentCapturePass
{
	bool initialize();
	bool update();
	bool resize(uint32_t newSizeX, uint32_t newSizeY);
	bool reloadShader();

	GLRenderPassComponent* getGLRPC();

	const std::vector<Probe>& getProbes();
	const std::vector<Brick>& getBricks();
}