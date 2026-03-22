//
//  MTGraphicsServiceBridgeImpl.mm
//  Main
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MTGraphicsServiceBridgeImpl.h"

using namespace Inno;
bool MTGraphicsServiceBridgeImpl::Setup() {
    [m_metalDelegate createDevice];

    [m_metalDelegate createView:[m_macWindowDelegate getFrame]];

    return true;
}

bool MTGraphicsServiceBridgeImpl::Initialize() {
    [m_metalDelegate createLibrary];

    [m_metalDelegate createPipeline];

    [m_metalDelegate createBuffer];
    return true;
}

bool MTGraphicsServiceBridgeImpl::Update() {
    return true;
}

bool MTGraphicsServiceBridgeImpl::render() {
    [m_metalDelegate render];
    return true;
}

bool MTGraphicsServiceBridgeImpl::present() {
    return true;
}

bool MTGraphicsServiceBridgeImpl::Terminate() {
    return true;
}

ObjectStatus MTGraphicsServiceBridgeImpl::GetStatus() {
    return m_ObjectStatus;
}

bool MTGraphicsServiceBridgeImpl::resize() {
    return true;
}

MTGraphicsServiceBridgeImpl::MTGraphicsServiceBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate *metalDelegate) {
    m_macWindowDelegate = macWindowDelegate;
    m_metalDelegate = metalDelegate;
}


MTGraphicsServiceBridgeImpl::~MTGraphicsServiceBridgeImpl() {
}

bool MTGraphicsServiceBridgeImpl::initializeMTMeshComponent(MTMeshComponent *rhs) {
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

bool MTGraphicsServiceBridgeImpl::initializeMTTextureComponent(MTTextureComponent *rhs) {
        return true;
}
