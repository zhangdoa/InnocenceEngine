//
//  NSApplication.m
//  Main
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import "NSApplication.h"
#import "MacWindowDelegate.h"

#import "../ApplicationEntry/ApplicationEntry.h"
#import "MacWindowSystemBridgeImpl.h"
#import "MTRenderingServerBridgeImpl.h"

using namespace Inno;
@implementation NSApplication
MacWindowSystemBridgeImpl* m_macWindowSystemBridge;
MTRenderingServerBridgeImpl* m_metalRenderingServerBridge;
MacWindowDelegate* m_macWindowDelegate;
MetalDelegate* m_metalDelegate;

- (void)run
{
    [[NSNotificationCenter defaultCenter]
      postNotificationName:NSApplicationWillFinishLaunchingNotification
      object:NSApp];
    [[NSNotificationCenter defaultCenter]
     postNotificationName:NSApplicationDidFinishLaunchingNotification
     object:NSApp];
    m_macWindowDelegate = [MacWindowDelegate alloc];
    m_metalDelegate = [MetalDelegate alloc];

    m_macWindowSystemBridge = new MacWindowSystemBridgeImpl(m_macWindowDelegate, m_metalDelegate);
    m_metalRenderingServerBridge = new MTRenderingServerBridgeImpl(m_macWindowDelegate, m_metalDelegate);

    //Start the engine C++ module
    const char* l_args = "-renderer 4 -mode 0 -loglevel 1";
    if (!Inno::ApplicationEntry::Setup(m_macWindowSystemBridge, m_metalRenderingServerBridge, (char*)l_args))
    {
        return;
    }
    if (!Inno::ApplicationEntry::Initialize())
    {
        return;
    }

    Inno::ApplicationEntry::Run();

    Inno::ApplicationEntry::Terminate();

    delete m_metalRenderingServerBridge;
    delete m_macWindowSystemBridge;
}

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)theApplication
{
    return YES;
}

- (void)applicationWillTerminate:(NSNotification *)aNotification{
    [m_macWindowDelegate windowWillClose:aNotification];
}
@end
