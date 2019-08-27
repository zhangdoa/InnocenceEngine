#version 450

layout(binding = 11, std140) uniform GISkyCBuffer
{
    layout(row_major) mat4 GISky_p_inv;
    layout(row_major) mat4 GISky_v_inv[6];
    vec4 GISky_probeCount;
    vec4 GISky_probeInterval;
    vec4 GISky_workload;
    vec4 GISky_irradianceVolumeOffset;
} _67;

uniform sampler3D SPIRV_Cross_CombinedprobeVolumeSPIRV_Cross_DummySampler;

layout(location = 0) in vec4 input_probeIndex;
layout(location = 1) in vec4 input_normal;
layout(location = 0) out vec4 _entryPointOutput_probeTestPassRT0;

void main()
{
    vec3 _300 = normalize(input_normal.xyz);
    vec3 _303 = _300 * _300;
    ivec3 _306 = mix(ivec3(0), ivec3(1), lessThan(_300, vec3(0.0)));
    vec3 _474;
    if (_306.x != int(0u))
    {
        _474 = vec3((texelFetch(SPIRV_Cross_CombinedprobeVolumeSPIRV_Cross_DummySampler, ivec3(uvec3(input_probeIndex.xyz + vec3(0.0, 0.0, _67.GISky_probeCount.z))), 0) * _303.x).xyz);
    }
    else
    {
        _474 = vec3((texelFetch(SPIRV_Cross_CombinedprobeVolumeSPIRV_Cross_DummySampler, ivec3(uvec3(input_probeIndex.xyz)), 0) * _303.x).xyz);
    }
    vec3 _475;
    if (_306.y != int(0u))
    {
        _475 = _474 + vec3((texelFetch(SPIRV_Cross_CombinedprobeVolumeSPIRV_Cross_DummySampler, ivec3(uvec3(input_probeIndex.xyz + vec3(0.0, 0.0, _67.GISky_probeCount.z * 3.0))), 0) * _303.y).xyz);
    }
    else
    {
        _475 = _474 + vec3((texelFetch(SPIRV_Cross_CombinedprobeVolumeSPIRV_Cross_DummySampler, ivec3(uvec3(input_probeIndex.xyz + vec3(0.0, 0.0, _67.GISky_probeCount.z * 2.0))), 0) * _303.y).xyz);
    }
    vec3 _476;
    if (_306.z != int(0u))
    {
        _476 = _475 + vec3((texelFetch(SPIRV_Cross_CombinedprobeVolumeSPIRV_Cross_DummySampler, ivec3(uvec3(input_probeIndex.xyz + vec3(0.0, 0.0, _67.GISky_probeCount.z * 5.0))), 0) * _303.z).xyz);
    }
    else
    {
        _476 = _475 + vec3((texelFetch(SPIRV_Cross_CombinedprobeVolumeSPIRV_Cross_DummySampler, ivec3(uvec3(input_probeIndex.xyz + vec3(0.0, 0.0, _67.GISky_probeCount.z * 4.0))), 0) * _303.z).xyz);
    }
    _entryPointOutput_probeTestPassRT0 = vec4(_476, 1.0);
}

