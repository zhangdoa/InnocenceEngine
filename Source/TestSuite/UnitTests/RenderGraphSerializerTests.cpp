#include "../Common/TestRunner.h"
#include "../../Engine/RenderGraph/RenderGraphSerializer.h"
#include "../../Engine/RenderGraph/RenderGraphEnumStrings.h"

using namespace Inno;

// Core fixtures (enum strings, BRDFLUT graph, buffer + imported, DrawModelGroups
// culling) live in RenderGraphSerializerTests_Core.cpp.
extern void TestEnumRoundTrip();
extern void TestGraphRoundTrip();
extern void TestBufferRoundTrip();
extern void TestComputeCullingRoundTrip();

// Deferred-RT / screen-sized + ScreenTile-dispatch round-trip fixtures live in
// RenderGraphSerializerTests_ScreenSized.cpp.
extern void TestScreenSizedRoundTrip();

// State-transition prepass round-trip fixture lives in
// RenderGraphTransitionTests.cpp.
extern void TestTransitionRoundTrip();

// SSAO node fixture (sampler bindings + 2-entry transition prepass + deferred
// screen Result) lives in RenderGraphSerializerTests_SSAO.cpp.
extern void TestSSAONodeRoundTrip();

// TiledFrustum node fixture (TiledTwoLevel dispatch mode + imported owned
// buffers + omitted transition set) lives in
// RenderGraphSerializerTests_TiledFrustum.cpp.
extern void TestTiledFrustumNodeRoundTrip();

// Ping-pong fixture (m_PingPong resource + m_PingPongHistory binding/transition)
// lives in RenderGraphSerializerTests_PingPong.cpp.
extern void TestPingPongNodeRoundTrip();

// Raster fixture (graphics-queue node: VS/PS shader paths + root-constant binding
// + raster pipeline block) lives in RenderGraphSerializerTests_Raster.cpp.
extern void TestRasterNodeRoundTrip();

// Raytracing fixture (compute-queue node: RT shader stages + TLAS binding +
// DispatchRays) lives in RenderGraphSerializerTests_Raytracing.cpp.
extern void TestRaytracingNodeRoundTrip();

// Bypass primitive fixture (Bypass.Enabled + ClearOnBypass + raster pipeline
// description in a declared-but-inert node) lives in
// RenderGraphSerializerTests_Bypass.cpp.
extern void TestBypassNodeRoundTrip();

// LightPass full-shape fixture (18-binding compute node: 3 light CBs + GBuffer/
// BRDF/SSAO/grid/cache/sun-shadow SRVs + TLAS + sampler + 2 RW outputs, 15 reads,
// 13 prepass transitions) lives in RenderGraphSerializerTests_LightPassFull.cpp.
extern void TestLightPassFullNodeRoundTrip();

extern void RunRenderGraphSerializerTiledUnitTests();
void RunRenderGraphSerializerUnitTests()
{
	TestRunner::StartTestSuite("RenderGraphSerializer");
	TestEnumRoundTrip();
	TestGraphRoundTrip();
	TestBufferRoundTrip();
	TestComputeCullingRoundTrip();
	TestScreenSizedRoundTrip();
	TestTransitionRoundTrip();
	TestSSAONodeRoundTrip();
	TestTiledFrustumNodeRoundTrip();
	TestPingPongNodeRoundTrip();
	TestRasterNodeRoundTrip();
	TestBypassNodeRoundTrip();
	TestLightPassFullNodeRoundTrip();
	RunRenderGraphSerializerTiledUnitTests();
	TestRunner::EndTestSuite();
}

