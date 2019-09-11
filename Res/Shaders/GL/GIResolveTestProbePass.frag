#version 450

layout(binding = 11, std140) uniform GISkyCBuffer
{
    layout(row_major) mat4 GISky_p_inv;
    layout(row_major) mat4 GISky_v_inv[6];
    vec4 GISky_probeCount;
    vec4 GISky_probeInterval;
    vec4 GISky_workload;
    vec4 GISky_irradianceVolumeOffset;
} _53;

layout(binding = 7, std140) uniform skyCBuffer
{
    layout(row_major) mat4 sky_p_inv;
    layout(row_major) mat4 sky_v_inv;
    vec4 sky_viewportSize;
    vec4 sky_posWSNormalizer;
    vec4 sky_padding[6];
} _63;

uniform sampler3D SPIRV_Cross_CombinedprobeVolumeSamplerTypeLinear;

layout(location = 0) in vec4 input_posWS;
layout(location = 1) in vec4 input_probeIndex;
layout(location = 2) in vec4 input_normal;
layout(location = 0) out vec4 _entryPointOutput_probeTestPassRT0;

void main()
{
    vec3 _325 = normalize(input_normal.xyz);
    vec3 _328 = _325 * _325;
    ivec3 _331 = mix(ivec3(0), ivec3(1), lessThan(_325, vec3(0.0)));
    vec3 _342 = (input_posWS.xyz - _53.GISky_irradianceVolumeOffset.xyz) / _63.sky_posWSNormalizer.xyz;
    vec3 _490 = _342;
    _490.z = _342.z * 0.16666667163372039794921875;
    vec3 _500;
    if (_331.x != int(0u))
    {
        _500 = vec3((texture(SPIRV_Cross_CombinedprobeVolumeSamplerTypeLinear, _490 + vec3(0.0, 0.0, 0.16666667163372039794921875)) * _328.x).xyz);
    }
    else
    {
        _500 = vec3((texture(SPIRV_Cross_CombinedprobeVolumeSamplerTypeLinear, _490) * _328.x).xyz);
    }
    vec3 _501;
    if (_331.y != int(0u))
    {
        _501 = _500 + vec3((texture(SPIRV_Cross_CombinedprobeVolumeSamplerTypeLinear, _490 + vec3(0.0, 0.0, 0.5)) * _328.y).xyz);
    }
    else
    {
        _501 = _500 + vec3((texture(SPIRV_Cross_CombinedprobeVolumeSamplerTypeLinear, _490 + vec3(0.0, 0.0, 0.3333333432674407958984375)) * _328.y).xyz);
    }
    vec3 _502;
    if (_331.z != int(0u))
    {
        _502 = _501 + vec3((texture(SPIRV_Cross_CombinedprobeVolumeSamplerTypeLinear, _490 + vec3(0.0, 0.0, 0.833333313465118408203125)) * _328.z).xyz);
    }
    else
    {
        _502 = _501 + vec3((texture(SPIRV_Cross_CombinedprobeVolumeSamplerTypeLinear, _490 + vec3(0.0, 0.0, 0.666666686534881591796875)) * _328.z).xyz);
    }
    _entryPointOutput_probeTestPassRT0 = vec4(_502, 1.0);
}

