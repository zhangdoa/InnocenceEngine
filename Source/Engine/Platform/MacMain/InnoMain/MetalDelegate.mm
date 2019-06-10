//
//  MetalDelegate.mm
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import "MetalDelegate.h"
#import <Foundation/Foundation.h>
#import <simd/simd.h>

typedef struct {
    matrix_float4x4 rotationMatrix;
} Uniforms;

typedef struct {
    vector_float4 position;
    vector_float4 color;
} VertexIn;

static const VertexIn vertexData[] =
{
    { { 0.5, -0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0} },
    { {-0.5, -0.5, 0.0, 1.0}, {0.0, 1.0, 0.0, 1.0} },
    { {-0.5,  0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0} },
    { { 0.5,  0.5, 0.0, 1.0}, {1.0, 1.0, 0.0, 1.0} },
    { { 0.5, -0.5, 0.0, 1.0}, {1.0, 0.0, 0.0, 1.0} },
    { {-0.5,  0.5, 0.0, 1.0}, {0.0, 0.0, 1.0, 1.0} }
};

static matrix_float4x4 rotationMatrix2D(float radians)
{
    float cos = cosf(radians);
    float sin = sinf(radians);
    return (matrix_float4x4) {
        .columns[0] = {  cos, sin, 0, 0 },
        .columns[1] = { -sin, cos, 0, 0 },
        .columns[2] = {    0,   0, 1, 0 },
        .columns[3] = {    0,   0, 0, 1 }
    };
}

@implementation MetalDelegate
    
    id<MTLDevice> _device;
    MTKView* _view;
    id<MTLLibrary> _library;
    id<MTLRenderPipelineState> _pipelineState;
    id<MTLBuffer> _vertexBuffer;
    id<MTLBuffer> _uniformBuffer;
    id<MTLCommandQueue> _commandQueue;
    
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
         * Metal setup: Vertices
         */
        _vertexBuffer = [_device newBufferWithBytes:vertexData
                                             length:sizeof(vertexData)
                                            options:MTLResourceCPUCacheModeDefaultCache];
        
        /*
         * Metal setup: Uniforms
         */
        _uniformBuffer = [_device newBufferWithLength:sizeof(Uniforms)
                                              options:MTLResourceCPUCacheModeWriteCombined];
        
        /*
         * Metal setup: Command queue
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
        double rotationAngle = fmod(CACurrentMediaTime(), 2.0 * M_PI);
        Uniforms uniformSrc = (Uniforms) {
            .rotationMatrix = rotationMatrix2D(rotationAngle)};
        void* uniformTgt = [_uniformBuffer contents];
        memcpy(uniformTgt, &uniformSrc, sizeof(Uniforms));
        
        MTLRenderPassDescriptor* passDescriptor = [_view currentRenderPassDescriptor];
        id<CAMetalDrawable> drawable = [_view currentDrawable];
        id<MTLCommandBuffer> commandBuffer = [_commandQueue commandBuffer];
        id<MTLRenderCommandEncoder> commandEncoder = [commandBuffer renderCommandEncoderWithDescriptor:passDescriptor];
        
        [commandEncoder setRenderPipelineState:_pipelineState];
        [commandEncoder setVertexBuffer:_vertexBuffer offset:0 atIndex:0];
        [commandEncoder setVertexBuffer:_uniformBuffer offset:0 atIndex:1];
        [commandEncoder drawPrimitives:MTLPrimitiveTypeTriangle vertexStart:0 vertexCount:6];
        [commandEncoder endEncoding];
        [commandBuffer presentDrawable:drawable];
        [commandBuffer commit];
}
    
- (MTKView *)getView { 
    return _view;
}

- (void)submitGPUData:(void *)vertices :(unsigned int)verticesSize {
    id<MTLBuffer> l_vertexBuffer = [_device newBufferWithBytes:vertices
                         length:verticesSize
                        options:MTLResourceCPUCacheModeDefaultCache];
    NSLog(@"VBO has been generated: %@", l_vertexBuffer);
}

@end
