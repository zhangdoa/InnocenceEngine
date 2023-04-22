//
//  MetalDelegate.mm
//  Main
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import "MetalDelegate.h"
#import <Foundation/Foundation.h>
#import <simd/simd.h>
#include "../../Common/GPUDataStructure.h"
#include "../../Component/MTMeshComponent.h"

#include "../../Interface/IEngine.h"

using namespace Inno;
extern IEngine* g_Engine;

@implementation MetalDelegate
    
    id<MTLDevice> _device;
    MTKView* _view;
    id<MTLLibrary> _library;
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLBuffer> _perFrameUBO;
    id<MTLBuffer> _meshUBO;
    id<MTLCommandQueue> _commandQueue;

    static id<MTLBuffer> createUBO(long size)
    {
        return [_device newBufferWithLength:size
                                    options:MTLResourceCPUCacheModeWriteCombined];
    }

    static void updateUBO(id<MTLBuffer> ubo, void* data, long size)
    {
        void* uniformTgt = [ubo contents];
        memcpy(uniformTgt, data, size);
    }

    static void encodeDrawCall(id<MTLRenderCommandEncoder> commandEncoder, MTMeshComponent* rhs)
    {
        id<MTLBuffer> l_vertexBuffer = (__bridge_transfer id<MTLBuffer>)(rhs->m_VBO);
        id<MTLBuffer> l_indexBuffer = (__bridge_transfer id<MTLBuffer>)(rhs->m_IBO);
        [commandEncoder setVertexBuffer:l_vertexBuffer offset:0 atIndex:0];
        [commandEncoder drawIndexedPrimitives:MTLPrimitiveTypeTriangle indexCount:rhs->m_indicesSize indexType:MTLIndexTypeUInt32 indexBuffer:l_indexBuffer indexBufferOffset:0 instanceCount:1];
    }

    - (void)createDevice
    {
        _device = MTLCreateSystemDefaultDevice();
    }
    
    - (void)createView:(NSRect)frame
    {
        _view = [[MTKView alloc] initWithFrame:frame
                                                device:_device];
        [_view setDelegate:self];
    }
    
    - (void)createLibrary
    {
        NSError* error;
        
        NSString* librarySrc = [NSString stringWithContentsOfFile:@"res//shaders//MT//library.metal" encoding:NSUTF8StringEncoding error:&error];
        if(!librarySrc) {
            [NSException raise:@"Failed to read shaders" format:@"%@", [error localizedDescription]];
        }
        
        _library = [_device newLibraryWithSource:librarySrc options:nil error:&error];
        if(!_library) {
            [NSException raise:@"Failed to compile shaders" format:@"%@", [error localizedDescription]];
        }
    }
    
    - (void)createPipeline
    {
        NSError* error;
        
        id<CAMetalDrawable> drawable = [_view currentDrawable];
        
        MTLRenderPipelineDescriptor* pipelineDesc = [[MTLRenderPipelineDescriptor alloc] init];
        pipelineDesc.vertexFunction = [_library newFunctionWithName:@"vertexFunction"];
        pipelineDesc.fragmentFunction = [_library newFunctionWithName:@"fragmentFunction"];
        pipelineDesc.colorAttachments[0].pixelFormat = drawable.texture.pixelFormat;
        
        _pipelineState = [_device newRenderPipelineStateWithDescriptor:pipelineDesc error:&error];
        if(!_pipelineState) {
            [NSException raise:@"Failed to create pipeline state" format:@"%@", [error localizedDescription]];
        }
    }
    
    - (void)createBuffer
    {
        /*
         * Metal Setup: Vertices
         */
        /*
         * Metal Setup: Uniforms
         */
        _perFrameUBO = createUBO(sizeof(PerFrameConstantBuffer));
        
        /*
         * Metal Setup: Command queue
         */
        _commandQueue = [_device newCommandQueue];
    }
    
- (void)mtkView:(MTKView*)view drawableSizeWillChange:(CGSize)size
    {
        // Window is not resizable
        (void)view;
        (void)size;
    }

- (void)drawInMTKView:(nonnull MTKView *)view {
}

    
- (void)render
{
        auto l_perFrameGPUData = g_Engine->getRenderingFrontend()->GetPerFrameConstantBuffer();
        updateUBO(_perFrameUBO, &l_perFrameGPUData, sizeof(l_perFrameGPUData));
    
        MTLRenderPassDescriptor* passDescriptor = [_view currentRenderPassDescriptor];
        id<CAMetalDrawable> drawable = [_view currentDrawable];
        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
    
        [commandEncoder setRenderPipelineState:_pipelineState];
        [commandEncoder setFrontFacingWinding:MTLWindingCounterClockwise];
        [commandEncoder setCullMode:MTLCullModeFront];
    
        auto l_MeshComp = reinterpret_cast<MTMeshComponent*>(g_Engine->getRenderingFrontend()->GetMeshComponent(ProceduralMeshShape::Square));
        encodeDrawCall(commandEncoder, l_MeshComp);
        [commandEncoder endEncoding];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
}
    
- (MTKView *)getView {
    return _view;
}

- (void)submitGPUData:(void *)MeshComp{
    auto l_MeshComp = reinterpret_cast<MTMeshComponent*>(MeshComp);
    
    void* vertices = &l_MeshComp->m_vertices[0];
    auto verticesSize =l_MeshComp->m_vertices.size() * sizeof(Vertex);
    id<MTLBuffer> l_vertexBuffer = [_device newBufferWithBytes:vertices
                         length:verticesSize
                        options:MTLResourceCPUCacheModeDefaultCache];
    NSLog(@"VBO has been generated: %@", l_vertexBuffer);
    
    void* indices = &l_MeshComp->m_indices[0];
    auto indicesSize =l_MeshComp->m_indices.size() * sizeof(Index);
    id<MTLBuffer> l_indexBuffer = [_device newBufferWithBytes:indices
                                                        length:indicesSize
                                                       options:MTLResourceCPUCacheModeDefaultCache];
    NSLog(@"IBO has been generated: %@", l_indexBuffer);
    
    l_MeshComp->m_VBO = (__bridge_retained void*)l_vertexBuffer;
    l_MeshComp->m_IBO = (__bridge_retained void*)l_indexBuffer;
    l_MeshComp->m_ObjectStatus = ObjectStatus::Activated;
}

@end
