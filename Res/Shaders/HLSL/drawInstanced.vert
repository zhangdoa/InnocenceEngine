// shadertype=hlsl
float4 main(uint instanceID : SV_InstanceID) : SV_Position
{
    float2 positions[3] =
    {
        float2(-0.5, -0.5),
        float2( 0.0,  0.5),
        float2( 0.5, -0.5)
    };
    return float4(positions[instanceID % 3], 0.0, 1.0);
}
