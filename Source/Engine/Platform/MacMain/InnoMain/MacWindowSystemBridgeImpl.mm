//
//  MacWindowSystemBridgeImpl.mm
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MacWindowSystemBridgeImpl.h"

bool MacWindowSystemBridgeImpl::setup(unsigned int sizeX, unsigned int sizeY) {
    NSRect frame = NSMakeRect(0, 0, sizeX, sizeY);
    
    [m_macWindowDelegate initWithContentRect:frame
                                   styleMask:NSWindowStyleMaskTitled|NSWindowStyleMaskClosable
                                     backing:NSBackingStoreBuffered
                                       defer:NO];
    return true;
}

bool MacWindowSystemBridgeImpl::initialize() {
    [m_macWindowDelegate setView:[m_metalDelegate getView]];
    return true;
}

bool MacWindowSystemBridgeImpl::update() {
    return true;
}

bool MacWindowSystemBridgeImpl::terminate() {
    return true;
}

ObjectStatus MacWindowSystemBridgeImpl::getStatus() {
    return m_objectStatus;
}

MacWindowSystemBridgeImpl::MacWindowSystemBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate *metalDelegate) {
    m_macWindowDelegate = macWindowDelegate;
    m_metalDelegate = metalDelegate;
}


MacWindowSystemBridgeImpl::~MacWindowSystemBridgeImpl() {
}
