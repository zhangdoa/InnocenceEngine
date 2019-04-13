//
//  MetalDelegate.h
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MetalDelegate_h
#define MetalDelegate_h

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

@interface MetalDelegate : NSObject <MTKViewDelegate> {
}
    - (void)createDevice;
    - (MTKView*)createView:(NSRect)frame;
    - (void)createLibrary;
    - (void)createPipeline:(id<CAMetalDrawable>)drawable;
    - (void)createBuffer;
    @end

#endif /* MetalDelegate_h */
