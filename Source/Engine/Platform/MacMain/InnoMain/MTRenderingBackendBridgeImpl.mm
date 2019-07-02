//
//  MTRenderingBackendBridgeImpl.mm
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#import <Foundation/Foundation.h>
#import "MTRenderingBackendBridgeImpl.h"

bool MTRenderingBackendBridgeImpl::setup() {
    [m_metalDelegate createDevice];
    
    [m_metalDelegate createView:[m_macWindowDelegate getFrame]];
    
    return true;
}

bool MTRenderingBackendBridgeImpl::initialize() {
    [m_metalDelegate createLibrary];
    
    [m_metalDelegate createPipeline];
    
    [m_metalDelegate createBuffer];
    return true;
}

bool MTRenderingBackendBridgeImpl::update() {
    return true;
}

bool MTRenderingBackendBridgeImpl::render() {
    [m_metalDelegate render];
    return true;
}

bool MTRenderingBackendBridgeImpl::terminate() {
    return true;
}

ObjectStatus MTRenderingBackendBridgeImpl::getStatus() {
    return m_objectStatus;
}

bool MTRenderingBackendBridgeImpl::resize() {
    return true;
}

bool MTRenderingBackendBridgeImpl::reloadShader(RenderPassType renderPassType) {
    return true;
}

bool MTRenderingBackendBridgeImpl::bakeGI() {
    return true;
}

MTRenderingBackendBridgeImpl::MTRenderingBackendBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate *metalDelegate) {
    m_macWindowDelegate = macWindowDelegate;
    m_metalDelegate = metalDelegate;
}


MTRenderingBackendBridgeImpl::~MTRenderingBackendBridgeImpl() {
}

bool MTRenderingBackendBridgeImpl::initializeMTMeshDataComponent(MTMeshDataComponent *rhs) {
    if (rhs->m_objectStatus == ObjectStatus::Activated)
    {
        return true;
    }
    else
    {
        [m_metalDelegate submitGPUData:&rhs->m_vertices[0]:(unsigned int)(rhs->m_vertices.size())];
        
        rhs->m_objectStatus = ObjectStatus::Activated;
        
        return true;
    }
}

bool MTRenderingBackendBridgeImpl::initializeMTTextureDataComponent(MTTextureDataComponent *rhs) {
        return true;
}
