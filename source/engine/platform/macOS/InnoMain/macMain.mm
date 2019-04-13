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
@property (nonatomic, retain) NSOpenGLView* glView;
-(void) drawLoop:(NSTimer*) timer;
@end

@implementation MacEntryWrapper

@synthesize glView;

BOOL shouldStop = NO;

-(id)initWithContentRect:(NSRect)contentRect styleMask:(NSUInteger)aStyle backing:(NSBackingStoreType)bufferingType defer:(BOOL)flag{
    if(self = [super initWithContentRect:contentRect styleMask:aStyle backing:bufferingType defer:flag]){
        //sets the title of the window (Declared in Plist)
        [self setTitle:[[NSProcessInfo processInfo] processName]];
        
        //This is pretty important.. OS X starts always with a context that only supports openGL 2.1
        //This will ditch the classic OpenGL and initialises openGL 4.1
        NSOpenGLPixelFormatAttribute pixelFormatAttributes[] ={
            NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
            NSOpenGLPFAColorSize    , 24                           ,
            NSOpenGLPFAAlphaSize    , 8                            ,
            NSOpenGLPFADoubleBuffer ,
            NSOpenGLPFAAccelerated  ,
            NSOpenGLPFANoRecovery   ,
            0
        };
        
        NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc]initWithAttributes:pixelFormatAttributes];
        //Initialize the view
        glView = [[NSOpenGLView alloc]initWithFrame:contentRect pixelFormat:format];
        
        //Set context and attach it to the window
        [[glView openGLContext]makeCurrentContext];
        
        //finishing off
        [self setContentView:glView];
        [glView prepareOpenGL];
        [self makeKeyAndOrderFront:self];
        [self setAcceptsMouseMovedEvents:YES];
        [self makeKeyWindow];
        [self setOpaque:YES];
        
        //Start the c++ code
        const char* l_args = "-renderer 0 -mode 0";
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
        if (!InnoApplication::update())
        {
            InnoApplication::terminate();
            return;
        }
        return;
    }
    if([self isVisible]){
        [glView update];
        [[glView openGLContext] flushBuffer];
    }
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication     *)theApplication{
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification{
    shouldStop = YES;
}
#pragma mark -
#pragma mark Cleanup
- (void) dealloc
{
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
