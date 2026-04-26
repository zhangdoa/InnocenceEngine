// shadertype=hlsl
// AgX tonemap — polynomial-fit display transform.
//
// Reference: Troy Sobotka's AgX
//   [https://github.com/sobotka/AgX]
// Polynomial / matrix port follows the Three.js implementation
//   [https://github.com/mrdoob/three.js/blob/dev/src/renderers/shaders/ShaderChunk/tonemapping_pars_fragment.glsl.js]
// which itself follows Filament's tonemap_AgX
//   [https://github.com/google/filament/blob/main/filament/src/ToneMapper.cpp].
//
// Pipeline:
//   1. NaN/Inf guard (loud-fail-quiet: clamp to zero, debug captures will read black).
//   2. Convert linear-sRGB scene-referred radiance into AgX log-encoded space
//      (matrix multiply + EV-clamped log2 normalised to AgX min/max EV).
//   3. Apply the 6th-order polynomial sigmoid that approximates the AgX "Default"
//      contrast LUT.
//   4. Inverse-transform back to display-referred sRGB-linear (the caller's
//      AccurateLinearToSRGB step then encodes for display).

// AgX log range. Scene EV is clamped to [-12.47, +4.026] before the sigmoid.
// Source: Three.js AgX implementation — same numbers as Filament & Sobotka's
// reference notebook (AgX-default config).
static const float AGX_MIN_EV  = -12.47393f;
static const float AGX_MAX_EV  =   4.026069f;
static const float AGX_EV_SPAN = AGX_MAX_EV - AGX_MIN_EV;

// Linear sRGB -> AgX log encoding matrix. Row-major as authored; we use it
// row-vector × matrix in HLSL (engine convention).
static const float3x3 AGX_INPUT_MATRIX = float3x3(
    0.842479062253094f,  0.0423282422610123f, 0.0423756549057051f,
    0.0784335999999992f, 0.878468636469772f,  0.0784336f,
    0.0792237451477643f, 0.0791661274605434f, 0.879142973793104f);

// Inverse: AgX log -> linear sRGB after the sigmoid.
static const float3x3 AGX_OUTPUT_MATRIX = float3x3(
     1.19687900512017f,   -0.0528968517574562f, -0.0529716355144438f,
    -0.0980208811401368f,  1.15190312990417f,   -0.0980434501171241f,
    -0.0990297440797205f, -0.0989611768448433f,  1.15107367264116f);

// 6th-order polynomial fit of the AgX "Default" sigmoid contrast curve.
// Coefficients from Three.js / Filament; equivalent to evaluating Sobotka's
// reference 1D LUT to within float32 precision.
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

    // The polynomial overshoots [0,1] slightly at the extremes; clamp before
    // gamma so the LDR encode stage does not see negatives.
    return saturate(v);
}
