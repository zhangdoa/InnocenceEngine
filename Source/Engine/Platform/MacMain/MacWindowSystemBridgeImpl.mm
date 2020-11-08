//
//  MacWindowSystemBridgeImpl.mm
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MacWindowSystemBridgeImpl.h"

using namespace Inno;
bool MacWindowSystemBridgeImpl::Setup(uint32_t sizeX, uint32_t sizeY) {
    NSRect frame = NSMakeRect(0, 0, sizeX, sizeY);
    
    [m_macWindowDelegate initWithContentRect:frame
                                   styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
                                     backing:NSBackingStoreBuffered
                                       defer:NO];
    
    NSDictionary *infoDictionary = [[NSBundle mainBundle] infoDictionary];
    Class principalClass =
    NSClassFromString([infoDictionary objectForKey:@"NSPrincipalClass"]);
    app = [principalClass sharedApplication];
    
    m_ObjectStatus = ObjectStatus::Created;
    return true;
}

bool MacWindowSystemBridgeImpl::Initialize() {
    [m_macWindowDelegate setView:[m_metalDelegate getView]];
        m_ObjectStatus = ObjectStatus::Activated;
    return true;
}

bool MacWindowSystemBridgeImpl::Update() {
    NSEvent *event =
    [app
     nextEventMatchingMask:NSEventMaskAny
     untilDate:[NSDate distantFuture]
     inMode:NSDefaultRunLoopMode
     dequeue:YES];
    
    [app sendEvent:event];
    [app updateWindows];
    return true;
}

bool MacWindowSystemBridgeImpl::Terminate() {
    return true;
}

ObjectStatus MacWindowSystemBridgeImpl::GetStatus() {
    if(![m_macWindowDelegate isAlive])
    {
        m_ObjectStatus = ObjectStatus::Suspended;
    }
    return m_ObjectStatus;
}

MacWindowSystemBridgeImpl::MacWindowSystemBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate *metalDelegate) {
    m_macWindowDelegate = macWindowDelegate;
    m_metalDelegate = metalDelegate;
}


MacWindowSystemBridgeImpl::~MacWindowSystemBridgeImpl() {
}
