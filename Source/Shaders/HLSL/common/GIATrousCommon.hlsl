// shadertype=hlsl
#ifndef GI_ATROUS_COMMON_HLSL
#define GI_ATROUS_COMMON_HLSL

#include "common/common.hlsl"

#ifndef ATROUS_STRIDE
#error "Define ATROUS_STRIDE before including GIATrousCommon.hlsl"
#endif

// SVGF (Schied et al. 2017) à-trous wavelet filter body. The caller
// wraps this with a specific ATROUS_STRIDE (1, 2, 4) so the compiler
// bakes the tap offsets and unrolls the loops; three separate shader
// objects is the simplest way to get that without runtime branching on
// a CB value.

// 1D binomial kernel (SVGF original). 2D tensor product gives the
// standard 5x5 à-trous stencil.
static const float g_AtrousKernel[5] = { 1.0/16.0, 1.0/4.0, 3.0/8.0, 1.0/4.0, 1.0/16.0 };

// Edge-stopping sigmas — SVGF defaults. σ_N high = narrow normal tolerance
// (128 gives ~3° effective cone). σ_L scales the luminance weight against
// the per-pixel temporal std-dev, so a high-variance (unconverged) pixel
// accepts more distant luma taps than a converged one.
static const float g_SigmaNormal = 128.0;
static const float g_SigmaLuma = 4.0;
// Depth error budget — matches the temporal pass's reprojection gate so
// the temporal + spatial filters reject the same discontinuities.
static const float g_DepthErrorBudget = 0.1;
// SVGF §2.4.3 disocclusion dilation. Pixels with N ≥ N_LOW have converged
// temporal variance and use σ_L directly. N below that = fresh / halo;
// their σ_L gets boosted so the A-trous filter can cross the halo
// instead of luma-rejecting against noisy neighbours.
static const float g_NLowThreshold = 4.0;
static const float g_LowNLumaBoost = 4.0;

struct ComputeInputType { uint3 dispatchThreadID : SV_DispatchThreadID; };

[[vk::binding(0, 0)]] cbuffer PerFrameConstantBuffer : register(b0)
{
	PerFrame_CB g_Frame;
}

[[vk::binding(0, 1)]] Texture2D in_opaquePassRT0 : register(t0);
[[vk::binding(1, 1)]] Texture2D in_opaquePassRT1 : register(t1);
[[vk::binding(2, 1)]] Texture2D<float4> in_Color : register(t2);
[[vk::binding(3, 1)]] Texture2D<float4> in_Moments : register(t3);

[[vk::binding(0, 2)]] RWTexture2D<float4> out_Color : register(u0);

struct VarianceSample
{
	float variance; // 3×3-Gaussian-smoothed σ² when minN is low, else raw.
	float minN;     // dilated history count (min over 3×3 neighbourhood).
};

// Raw σ² = E[L²] − E[L]² underestimates at low history counts (< 4
// frames), so we pull in the 3×3 neighbourhood average when the
// dilated N is small. The same neighbourhood scan does the SVGF §2.4.3
// disocclusion dilation: a pixel adjacent to a newly-disoccluded
// neighbour has its effective N clamped down so the caller widens σ_L
// and the à-trous filter crosses the halo instead of luma-rejecting
// against the disoccluded neighbour's noisy single sample.
VarianceSample SampleVariance(int2 p, int2 viewport)
{
	float4 m = in_Moments.Load(int3(p, 0));
	float centerVar = max(m.y - m.x * m.x, 0.0);
	float minN = m.z;

	// First pass: find dilated minN across the 3×3 neighbourhood.
	[unroll] for (int dyN = -1; dyN <= 1; dyN++)
	{
		[unroll] for (int dxN = -1; dxN <= 1; dxN++)
		{
			if (dxN == 0 && dyN == 0) continue;
			int2 qN = p + int2(dxN, dyN);
			if (any(qN < int2(0, 0)) || any(qN >= viewport)) continue;
			minN = min(minN, in_Moments.Load(int3(qN, 0)).z);
		}
	}

	VarianceSample r;
	r.minN = minN;

	if (minN >= g_NLowThreshold)
	{
		r.variance = centerVar;
		return r;
	}

	static const float k[3] = { 1.0/4.0, 1.0/2.0, 1.0/4.0 };
	float sum = 0.0;
	float wsum = 0.0;
	[unroll] for (int dy = -1; dy <= 1; dy++)
	{
		[unroll] for (int dx = -1; dx <= 1; dx++)
		{
			int2 q = p + int2(dx, dy);
			if (any(q < int2(0, 0)) || any(q >= viewport)) continue;
			float4 mq = in_Moments.Load(int3(q, 0));
			float v = max(mq.y - mq.x * mq.x, 0.0);
			float w = k[dx + 1] * k[dy + 1];
			sum += w * v;
			wsum += w;
		}
	}
	r.variance = (wsum > 0.0) ? sum / wsum : centerVar;
	return r;
}

[numthreads(8, 8, 1)]
void main(ComputeInputType input)
{
	int2 p = int2(input.dispatchThreadID.xy);
	int2 viewport = int2(g_Frame.viewportSize.xy);
	if (any(p >= viewport)) return;

	float4 pos4 = in_opaquePassRT0.Load(int3(p, 0));
	// Sky / no geometry: pass through the same sentinel the temporal
	// stage writes so downstream stages and LightPass keep agreeing.
	if (pos4.a == 0.0)
	{
		out_Color[p] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	float3 posP = pos4.xyz;
	float3 normP = normalize(in_opaquePassRT1.Load(int3(p, 0)).xyz);
	float depthP = length(posP - g_Frame.camera_posWS.xyz);

	float4 centerColor = in_Color.Load(int3(p, 0));
	float3 colP = centerColor.rgb;
	float lumaP = GetLuma(colP);

	VarianceSample varSample = SampleVariance(p, viewport);
	float stdP = sqrt(varSample.variance);
	// Luminance weight normaliser. EPSILON keeps weights bounded when
	// stdP is 0 (fully-converged region); without it an unchanged pixel
	// would reject all taps with any luma delta. Low dilated-N (halo
	// around a disocclusion) widens σ_L so the filter can blend across
	// the halo — at those pixels the temporal variance is unreliable
	// anyway, so the spatial filter is the only thing reducing noise.
	float lowNBoost = (varSample.minN < g_NLowThreshold) ? g_LowNLumaBoost : 1.0;
	float lumaSigma = g_SigmaLuma * stdP * lowNBoost + EPSILON;

	float centerW = g_AtrousKernel[2] * g_AtrousKernel[2];
	float3 sumCol = colP * centerW;
	float sumW = centerW;

	[unroll] for (int dy = -2; dy <= 2; dy++)
	{
		[unroll] for (int dx = -2; dx <= 2; dx++)
		{
			if (dx == 0 && dy == 0) continue;

			int2 q = p + int2(dx, dy) * ATROUS_STRIDE;
			if (any(q < int2(0, 0)) || any(q >= viewport)) continue;

			float4 posQ4 = in_opaquePassRT0.Load(int3(q, 0));
			if (posQ4.a == 0.0) continue;

			float3 posQ = posQ4.xyz;
			float depthQ = length(posQ - g_Frame.camera_posWS.xyz);
			float depthErr = abs(depthQ - depthP) / max(depthP, EPSILON);
			if (depthErr > g_DepthErrorBudget) continue;

			float3 normQ = normalize(in_opaquePassRT1.Load(int3(q, 0)).xyz);
			float nDot = max(0.0, dot(normP, normQ));

			float4 colQ4 = in_Color.Load(int3(q, 0));
			float lumaQ = GetLuma(colQ4.rgb);

			float wZ = exp(-depthErr * 10.0);
			float wN = pow(nDot, g_SigmaNormal);
			float wL = exp(-abs(lumaP - lumaQ) / lumaSigma);

			float kernelW = g_AtrousKernel[dx + 2] * g_AtrousKernel[dy + 2];
			float w = kernelW * wZ * wN * wL;

			sumCol += w * colQ4.rgb;
			sumW += w;
		}
	}

	float3 filtered = sumCol / max(sumW, EPSILON);
	// Preserve the linear-depth alpha channel so downstream stages that
	// reproject against this output (none today, but the same contract
	// as the temporal pass) stay consistent.
	out_Color[p] = float4(filtered, centerColor.a);
}

#endif // GI_ATROUS_COMMON_HLSL
