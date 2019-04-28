//
//  MTRenderingSystemBridgeImpl.mm
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MTRenderingSystemBridgeImpl.h"

bool MTRenderingSystemBridgeImpl::setup() {
    [m_metalDelegate createDevice];
    
    [m_metalDelegate createView:[m_macWindowDelegate getFrame]];
    
    return true;
}

bool MTRenderingSystemBridgeImpl::initialize() {
    [m_metalDelegate createLibrary];
    
    [m_metalDelegate createPipeline];
    
    [m_metalDelegate createBuffer];
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

MTRenderingSystemBridgeImpl::MTRenderingSystemBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate *metalDelegate) {
    m_macWindowDelegate = macWindowDelegate;
    m_metalDelegate = metalDelegate;
}


MTRenderingSystemBridgeImpl::~MTRenderingSystemBridgeImpl() {
}

bool MTRenderingSystemBridgeImpl::initializeMTMeshDataComponent(MTMeshDataComponent *rhs) {
    if (rhs->m_objectStatus == ObjectStatus::ALIVE)
    {
        return true;
    }
    else
    {
        [m_metalDelegate submitGPUData:rhs->m_vertices.data() :(unsigned int)rhs->m_vertices.size()];
        
        rhs->m_objectStatus = ObjectStatus::ALIVE;
        
        return true;
    }
}

bool MTRenderingSystemBridgeImpl::initializeMTTextureDataComponent(MTTextureDataComponent *rhs) { 
        return true;
}


