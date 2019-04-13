//
//  macMain.mm
//  macMain
//
//  Created by zhangdoa on 13/04/2019.
//  Copyright © 2019 zhangdoa. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Cocoa/Cocoa.h>
#import "../../../common/InnoApplication.h"

NSApplication* application;

@interface MacEntryWrapper : NSWindow <NSApplicationDelegate>{
}
-(void) drawLoop:(NSTimer*) timer;
@end

@implementation MacEntryWrapper

BOOL shouldStop = NO;

-(id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag{
    if(self = [super initWithContentRect:contentRect styleMask:aStyle backing:bufferingType defer:flag]){
        
        //sets the title of the window (Declared in Plist)
        [self setTitle:[[NSProcessInfo processInfo] processName]];
        
        //finishing off
        [self makeKeyAndOrderFront:self];
        [self setAcceptsMouseMovedEvents:YES];
        [self makeKeyWindow];
        [self setOpaque:YES];
        
        //Start the c++ code
        const char* l_args = "-renderer 4 -mode 0";
        if (!InnoApplication::setup(nullptr, (char*)l_args))
        {
            return 0;
        }
        if (!InnoApplication::initialize())
        {
            return 0;
        }
    }
    return self;
}

-(void) drawLoop:(NSTimer*) timer{
    if(shouldStop){
        [self close];
        return;
    }
    if([self isVisible]){
        if (!InnoApplication::update())
        {
            shouldStop = YES;
            InnoApplication::terminate();
            return;
        }
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication{
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification{
    shouldStop = YES;
    InnoApplication::terminate();
}
#pragma mark -
#pragma mark Cleanup
- (void) dealloc
{
    InnoApplication::terminate();
}
@end

int main(int argc, const char * argv[]) {
    MacEntryWrapper* app;
    application = [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    //create a window with the size of 600 by 600
    app = [[MacEntryWrapper alloc] initWithContentRect:NSMakeRect(0, 0, 600, 600)              styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |  NSWindowStyleMaskMiniaturizable   backing:NSBackingStoreBuffered defer:YES];
    [application setDelegate:app];
    [application run];
    
    return NSApplicationMain(argc, argv);
}
