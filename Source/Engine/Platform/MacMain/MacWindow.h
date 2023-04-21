//
//  MacWindow.h
//  Main
//
//  Created by zhangdoa on 11/06/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import <Foundation/Foundation.h>

#import <Cocoa/Cocoa.h>
#import <MetalKit/MetalKit.h>

@interface MacWindow : NSWindow {
}
-(id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag;
-(NSRect)getFrame;
-(void)setView:(MTKView*) view;
-(BOOL)isAlive;

@end
