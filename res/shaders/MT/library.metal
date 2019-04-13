//
//  library.metal
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#include <metal_stdlib>

using namespace metal;

struct Uniforms
{
    float4x4 rotationMatrix;
};

struct VertexIn
{
    float4 position;
    float4 color;
};

struct VertexOut
{
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertexFunction(device VertexIn *vertices [[buffer(0)]],
                                constant Uniforms &uniforms [[buffer(1)]],
                                uint vid [[vertex_id]])
{
    VertexOut out;
    out.position = uniforms.rotationMatrix * vertices[vid].position;
    out.color = vertices[vid].color;
    return out;
}

fragment float4 fragmentFunction(VertexOut in [[stage_in]])
{
    return in.color;
}
