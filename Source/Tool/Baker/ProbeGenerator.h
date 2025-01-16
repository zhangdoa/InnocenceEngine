#pragma once
#include "Common.h"
#include "../../Engine/Engine.h"

namespace Inno
{
    namespace Baker
    {
        class ProbeGenerator
        {
            INNO_CLASS_SINGLETON(ProbeGenerator)

            void setup();
            bool gatherStaticMeshData();
            bool generateProbeCaches(std::vector<Probe>& probes);
            ProbeInfo generateProbes(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval);
            bool generateProbesAlongTheSurface(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval);
            uint32_t generateProbesAlongTheWall(std::vector<Probe>& probes, const std::vector<Vec4>& heightMap, uint32_t probeMapSamplingInterval);
            bool assignBrickFactorToProbesByGPU(const std::vector<Brick>& bricks, std::vector<Probe>& probes);

            RenderPassComponent* m_RenderPassComp_Probe;
            ShaderProgramComponent* m_SPC_Probe;
        };
    }
}
