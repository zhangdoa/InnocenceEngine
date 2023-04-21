#pragma once
#include "Common.h"
#include "../../Engine/Interface/IEngine.h"

namespace Inno
{
    namespace Baker
    {
        class SurfelGenerator
        {
            INNO_CLASS_SINGLETON(SurfelGenerator)

            void setup();
            bool captureSurfels(std::vector<Probe>& probes);
            bool drawObjects(Probe& probe, const Mat4& p, const std::vector<Mat4>& v);
            bool readBackSurfelCaches(Probe& probe, std::vector<Surfel>& surfelCaches);
            bool eliminateDuplicatedSurfels(std::vector<Surfel>& surfelCaches);

            RenderPassComponent* m_RenderPassComp_Surfel;
            ShaderProgramComponent* m_SPC_Surfel;
            SamplerComponent* m_SamplerComp_Surfel;
        };
    }
}
