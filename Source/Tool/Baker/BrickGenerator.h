#pragma once
#include "Common.h"
#include "../../Engine/Interface/IEngine.h"

namespace Inno
{
    namespace Baker
    {
        class BrickGenerator
        {
            INNO_CLASS_SINGLETON(BrickGenerator)

            void setup();
            bool generateBrickCaches(std::vector<Surfel>& surfelCaches);
            bool generateBricks(const std::vector<BrickCache>& brickCaches);
            bool assignBrickFactorToProbesByGPU(const std::vector<Brick>& bricks, std::vector<Probe>& probes);
            bool drawBricks(Vec4 pos, uint32_t bricksCount, const Mat4& p, const std::vector<Mat4>& v);
            bool readBackBrickFactors(Probe& probe, std::vector<BrickFactor>& brickFactors, const std::vector<Brick>& bricks);

            RenderPassComponent* m_RenderPassComp_BrickFactor;
            ShaderProgramComponent* m_SPC_BrickFactor;
        };
    }
}
