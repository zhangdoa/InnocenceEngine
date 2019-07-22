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
struct SurfelGrid
{
	vec4 pos;
	vec4 normal;
	vec4 albedo;
	vec4 MRAT;
};

// 4x4x4 m^3 of surfels
struct Brick
{
	AABB boundBox;
	unsigned int surfelRangeBegin;
	unsigned int surfelRangeEnd;
};

struct BrickFactor
{
	float basisWeight;
	unsigned int brickIndex;
};

struct Probe
{
	vec4 pos;
	SH9 skyVisibility;
	SH9 radiance;
	unsigned int brickFactorRangeBegin;
	unsigned int brickFactorRangeEnd;
};

namespace GLEnvironmentCapturePass
{
	bool initialize();
	bool update();
	bool resize(unsigned int newSizeX, unsigned int newSizeY);
	bool reloadShader();
	const std::pair<std::vector<vec4>, std::vector<SH9>>& getRadianceSH9();
	const std::pair<std::vector<vec4>, std::vector<SH9>>& getSkyVisibilitySH9();
	const std::vector<Brick>& getBricks();

	GLRenderPassComponent* getGLRPC();
}