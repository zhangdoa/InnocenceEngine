//
//  library.metal
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#include <metal_stdlib>

using namespace metal;

struct VertexIn
{
    float4 position;
    float2 texCoord;
    float2 pad1;
    float4 normal;
    float4 pad2;
};

struct VertexOut
{
    float4 position [[position]];
};

vertex VertexOut vertexFunction(device VertexIn *vertices [[buffer(0)]],
                                uint vid [[vertex_id]])
{
    VertexOut out;
    out.position = vertices[vid].position;
    return out;
}

fragment float4 fragmentFunction(VertexOut in [[stage_in]])
{
    return float4(0.1f, 0.2f, 0.3f, 1.0f);
}
