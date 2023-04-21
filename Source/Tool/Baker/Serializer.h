
#pragma once
#include "Common.h"

namespace Inno
{
    namespace Baker
    {
        bool serializeProbeInfos(const ProbeInfo& probeInfo);
        bool serializeProbes(const std::vector<Probe>& probes);
        bool serializeSurfels(const std::vector<Surfel>& surfels);
        bool serializeSurfelCaches(const std::vector<Surfel>& surfelCaches);
        bool serializeBrickCaches(const std::vector<BrickCache>& brickCaches);
        bool deserializeBrickCaches(const std::vector<BrickCacheSummary>& brickCacheSummaries, std::vector<BrickCache>& brickCaches);
        bool serializeBricks(const std::vector<Brick>& bricks);
        bool serializeBrickFactors(const std::vector<BrickFactor>& brickFactors);
    }
}