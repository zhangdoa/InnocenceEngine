#pragma once
#include "../../Engine/RenderingServer/IRenderingServer.h"

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
	unsigned int surfelRangeBegin;
	unsigned int surfelRangeEnd;

	bool operator==(const Brick &other) const
	{
		return (boundBox.m_center == other.boundBox.m_center);
	}
};

struct BrickFactor
{
	float basisWeight;
	unsigned int brickIndex;
};

struct Probe
{
	vec4 pos;
	unsigned int brickFactorRangeBegin;
	unsigned int brickFactorRangeEnd;
	float padding[2];
};

namespace GIBakePass
{
	bool Setup();
	bool Initialize();
	bool Bake();
	bool PrepareCommandList();
	bool ExecuteCommandList();
	bool Terminate();

	RenderPassDataComponent* GetRPDC();
	ShaderProgramComponent* GetSPC();
	const std::vector<Surfel>& GetSurfels();
	const std::vector<Brick>& GetBricks();
	const std::vector<BrickFactor>& GetBrickFactors();
	const std::vector<Probe>& GetProbes();
	unsigned int GetProbeDimension();
};
