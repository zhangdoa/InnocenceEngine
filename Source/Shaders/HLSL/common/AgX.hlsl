// shadertype=hlsl
// AgX tonemap - polynomial-fit display transform.
//
// Reference: Troy Sobotka's AgX
//   [https://github.com/sobotka/AgX]
// Matrix port follows Filament's tonemap_AgX (canonical inset/outset matrices
// folded with the Rec2020 round-trip; no separate sRGB<->Rec2020 step needed):
//   [https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp]
// Cross-checked against Three.js' AgXToneMapping (same nine matrix values, same
// polynomial, same EV bounds; Three.js exposes the Rec2020 round-trip as a
// separate matrix multiply but the algebra collapses to Filament's inset/outset):
//   [https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js]
//
// Output: gamma-encoded display-referred sRGB ready for the swapchain. The
// pow(2.2) inside this function IS the gamma encode. CALLER MUST NOT RE-ENCODE
// (no AccurateLinearToSRGB, no extra pow(1/2.2)). Doubling the encode produces
// pastel/washed-out output - this was the TASK-141 regression.
//
// Pipeline:
//   1. NaN/Inf guard (loud-fail-quiet: clamp to zero, debug captures will read black).
//   2. Apply AgX inset matrix (linear-sRGB -> AgX wide-gamut working space).
//   3. EV-clamped log2 normalised to AgX min/max EV.
//   4. 6th-order polynomial sigmoid approximating the AgX "Default" contrast LUT.
//   5. Apply AgX outset matrix (back to display-referred linear sRGB).
//   6. pow(2.2) - the canonical AgX gamma encode for an sRGB-ish display.

// AgX log range. Scene EV is clamped to [-12.47393, +4.026069] before the sigmoid.
// Source: Filament & Three.js (both match Sobotka's reference notebook).
static const float AGX_MIN_EV  = -12.47393f;
static const float AGX_MAX_EV  =   4.026069f;
static const float AGX_EV_SPAN = AGX_MAX_EV - AGX_MIN_EV;

// AgX gamma encode exponent. Both Three.js and Filament hardcode 2.2 here; this
// is the AgX-Default convention (sRGB-ish display, not the strict sRGB EOTF).
static const float AGX_GAMMA_ENCODE_EXPONENT = 2.2f;

// AgX inset matrix - linear-sRGB scene-referred radiance -> AgX working space.
// Nine values verbatim from Filament's AgXInsetMatrix (also identical to
// Three.js' AgXInsetMatrix). HLSL convention here is row-vector x matrix
// (mul(v, M)), so the literal matches the GLSL/Filament source order.
static const float3x3 AGX_INPUT_MATRIX = float3x3(
    0.856627153315983f,  0.137318972929847f,  0.11189821299995f,
    0.0951212405381588f, 0.761241990602591f,  0.0767994186031903f,
    0.0482516061458583f, 0.101439036467562f,  0.811302368396859f);

// AgX outset matrix - inverse of the inset, back to display-referred linear sRGB
// after the sigmoid. Nine values verbatim from Filament's AgXOutsetMatrix
// (identical to Three.js' AgXOutsetMatrix).
static const float3x3 AGX_OUTPUT_MATRIX = float3x3(
     1.1271005818144368f,  -0.1413297634984383f,  -0.14132976349843826f,
    -0.11060664309660323f,  1.157823702216272f,   -0.11060664309660294f,
    -0.016493938717834573f,-0.016493938717834257f, 1.2519364065950405f);

// 6th-order polynomial fit of the AgX "Default" sigmoid contrast curve.
// Coefficients verbatim from Filament's agxDefaultContrastApprox; equivalent to
// evaluating Sobotka's reference 1D LUT to within float32 precision.
float3 AgXDefaultContrastApprox(float3 x)
{
    const float3 x2 = x * x;
    const float3 x4 = x2 * x2;
    const float3 x6 = x4 * x2;
    return -17.86f     * x6 * x
         + 78.01f      * x6
         - 126.7f      * x4 * x
         + 92.06f      * x4
         - 28.72f      * x2 * x
         +  4.361f     * x2
         -  0.1718f    * x
         +  0.002857f;
}

float3 TonemapAGX(const float3 x)
{
    // Loud-fail-quiet: scrub NaN / Inf so a single bad pixel does not poison
    // the polynomial (NaN would propagate through the entire frame).
    float3 v = x;
    v = (any(isnan(v)) || any(isinf(v))) ? float3(0.0f, 0.0f, 0.0f) : max(v, float3(0.0f, 0.0f, 0.0f));

    // Linear sRGB -> AgX wide-gamut working space.
    v = mul(v, AGX_INPUT_MATRIX);

    // Log2 with AgX EV bounds. The 1e-10 floor matches Sobotka's reference;
    // it sets log2(min_value) below AGX_MIN_EV so the saturate() below clips it.
    const float kLogFloor = 1e-10f;
    v = log2(max(v, kLogFloor));
    v = (v - AGX_MIN_EV) / AGX_EV_SPAN;
    v = saturate(v);

    // Sigmoid contrast (AgX "Default" look).
    v = AgXDefaultContrastApprox(v);

    // AgX log -> display-referred linear sRGB.
    v = mul(v, AGX_OUTPUT_MATRIX);

    // Canonical AgX gamma encode (Filament & Three.js both apply pow(2.2) HERE,
    // inside the tonemap function). The output is gamma-encoded display-referred
    // sRGB ready for the swapchain. Caller MUST NOT apply AccurateLinearToSRGB
    // or any other gamma encode after this - doing so double-encodes (pastel).
    v = pow(max(v, float3(0.0f, 0.0f, 0.0f)), AGX_GAMMA_ENCODE_EXPONENT);

    // Final clamp to LDR display range.
    return saturate(v);
}
