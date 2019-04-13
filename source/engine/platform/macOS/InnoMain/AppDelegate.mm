//
//  AppDelegate.m
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import "AppDelegate.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

#import "../../../common/InnoApplication.h"
#import "../../../system/MTRenderingBackend/MTRenderingSystemBridge.h"

class MTRenderingSystemBridgeImpl : public MTRenderingSystemBridge
{
public:
    MTRenderingSystemBridgeImpl() {};
    ~MTRenderingSystemBridgeImpl() {};
    
    bool setup() override;
    bool initialize() override;
    bool update() override;
    bool terminate() override;
    
    ObjectStatus getStatus() override;
    
    bool resize() override;
    bool reloadShader(RenderPassType renderPassType) override;
    bool bakeGI() override;
private:
    ObjectStatus m_objectStatus = ObjectStatus::SHUTDOWN;
};

bool MTRenderingSystemBridgeImpl::setup() {
    return true;
}

bool MTRenderingSystemBridgeImpl::initialize() {
    return true;
}

bool MTRenderingSystemBridgeImpl::update() {
    return true;
}

bool MTRenderingSystemBridgeImpl::terminate() {
    return true;
}

ObjectStatus MTRenderingSystemBridgeImpl::getStatus() {
    return m_objectStatus;
}

bool MTRenderingSystemBridgeImpl::resize() {
    return true;
}

bool MTRenderingSystemBridgeImpl::reloadShader(RenderPassType renderPassType) {
    return true;
}

bool MTRenderingSystemBridgeImpl::bakeGI() {
    return true;
}

MacWindow* macWindow;
MetalDelegate* metalDelegate;

@implementation AppDelegate
    MTRenderingSystemBridge* m_bridge;
    
-(void) drawLoop:(NSTimer*) timer{
    if(![macWindow isAlive]){
        [macWindow close];
        InnoApplication::terminate();
        return;
    }
    if([macWindow isVisible]){
        if (!InnoApplication::update())
        {
            InnoApplication::terminate();
            return;
        }
    }
}
    
- (id)init
    {
        m_bridge = new MTRenderingSystemBridgeImpl();
        
        NSRect frame = NSMakeRect(0, 0, 640, 480);
        
        macWindow = [[MacWindow alloc] initWithContentRect:frame
                                                 styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
                                                   backing:NSBackingStoreBuffered
                                                     defer:NO];
        metalDelegate = [MetalDelegate alloc];
        
        [metalDelegate createDevice];
        MTKView* view = [metalDelegate createView:frame];
        
        [macWindow setView:view];
        
        [metalDelegate createLibrary];
        
        id<CAMetalDrawable> drawable = [view currentDrawable];
        
        [metalDelegate createPipeline:drawable];
        
        [metalDelegate createBuffer];
        
        //Start the engine c++ code
        const char* l_args = "-renderer 4 -mode 0";
        if (!InnoApplication::setup(m_bridge, (char*)l_args))
        {
            return 0;
        }
        if (!InnoApplication::initialize())
        {
            return 0;
        }
        
        [NSTimer scheduledTimerWithTimeInterval:0.000001 target:self selector:@selector(drawLoop:) userInfo:nil repeats:YES];
        
        return self;
    }
    
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication
    {
        return YES;
    }
    
    - (void)applicationWillTerminate:(NSNotification *)aNotification{
        InnoApplication::terminate();
        delete m_bridge;
        [macWindow applicationWillTerminate:aNotification];
    }
    @end
