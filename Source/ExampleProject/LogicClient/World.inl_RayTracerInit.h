#pragma once

namespace Inno
{
    struct WorldSystem;
    // Initialize the example's CPU path tracer reference. Sweep3 dropped
    // the RayTracer Setup+Initialize from World.inl; this restores the
    // pre-sweep3 behavior without growing World.inl past HEAD.
    void InitializeRayTracerForWorld(WorldSystem& world);
}
