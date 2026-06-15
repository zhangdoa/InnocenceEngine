#include "World.inl_RayTracerInit.h"
#include "World.inl"

#include "../../Engine/RayTracer/RayTracer.h"
#include "../../Engine/Engine.h"

using namespace Inno;

void Inno::InitializeRayTracerForWorld(WorldSystem&)
{
    RayTracerConfig l_rtCfg;
    auto* l_rayTracer = g_Engine->Get<RayTracer>();
    l_rayTracer->Setup(&l_rtCfg);
    l_rayTracer->Initialize();
}
