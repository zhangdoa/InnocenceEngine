//
//  MacWindowDelegate.h
//  Main
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MacWindowDelegate_h
#define MacWindowDelegate_h

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

@interface MacWindowDelegate : NSWindow <NSWindowDelegate>{
}
    -(id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag;
    -(NSRect)getFrame;
    -(void)setView:(MTKView*) view;
    -(BOOL)isAlive;
    @end

#endif /* MacWindowDelegate_h */
