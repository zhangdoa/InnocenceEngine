//
//  MTRenderingServerBridgeImpl.mm
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MTRenderingServerBridgeImpl.h"

bool MTRenderingServerBridgeImpl::setup() {
    [m_metalDelegate createDevice];

    [m_metalDelegate createView:[m_macWindowDelegate getFrame]];

    return true;
}

bool MTRenderingServerBridgeImpl::initialize() {
    [m_metalDelegate createLibrary];

    [m_metalDelegate createPipeline];

    [m_metalDelegate createBuffer];
    return true;
}

bool MTRenderingServerBridgeImpl::update() {
    return true;
}

bool MTRenderingServerBridgeImpl::render() {
    [m_metalDelegate render];
    return true;
}

bool MTRenderingServerBridgeImpl::present() {
    return true;
}

bool MTRenderingServerBridgeImpl::terminate() {
    return true;
}

ObjectStatus MTRenderingServerBridgeImpl::getStatus() {
    return m_ObjectStatus;
}

bool MTRenderingServerBridgeImpl::resize() {
    return true;
}

MTRenderingServerBridgeImpl::MTRenderingServerBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate *metalDelegate) {
    m_macWindowDelegate = macWindowDelegate;
    m_metalDelegate = metalDelegate;
}


MTRenderingServerBridgeImpl::~MTRenderingServerBridgeImpl() {
}

bool MTRenderingServerBridgeImpl::initializeMTMeshDataComponent(MTMeshDataComponent *rhs) {
    if (rhs->m_ObjectStatus == ObjectStatus::Activated)
    {
        return true;
    }
    else
    {
        [m_metalDelegate submitGPUData:rhs];
        return true;
    }
}

bool MTRenderingServerBridgeImpl::initializeMTTextureDataComponent(MTTextureDataComponent *rhs) {
        return true;
}
