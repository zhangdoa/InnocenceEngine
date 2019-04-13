//
//  MacWindowDelegate.m
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import "MacWindowDelegate.h"
#import <Foundation/Foundation.h>

@implementation MacWindow
    
    BOOL m_isAlive = YES;
    
-(id)initWithContentRect:(NSRect)contentRect styleMask:(NSWindowStyleMask)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag{
    if(self = [super initWithContentRect:contentRect styleMask:aStyle backing:bufferingType defer:flag]){
        
        //sets the title of the window (Declared in Plist)
        [self setTitle:[[NSProcessInfo processInfo] processName]];
        
        //finishing off
        [self center];
        //[self makeKeyAndOrderFront:NSApp];
        
        [self makeKeyAndOrderFront:self];
        [self setAcceptsMouseMovedEvents:YES];
        [self makeKeyWindow];
        [self setOpaque:YES];
    }
    return self;
}
    
    -(void) setView:(MTKView*) view{
        [self setContentView:view];
    }
        
- (void)applicationWillTerminate:(NSNotification *)aNotification{
    m_isAlive = NO;
}
    - (BOOL)isAlive {
        return m_isAlive;
    }
    
    @end
