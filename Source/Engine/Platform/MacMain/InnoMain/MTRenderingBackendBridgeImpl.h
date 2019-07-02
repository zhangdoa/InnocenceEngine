//
//  MTRenderingBackendBridgeImpl.h
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MTRenderingBackendBridgeImpl_h
#define MTRenderingBackendBridgeImpl_h

#import "../../../RenderingBackend/MTRenderingBackend/MTRenderingBackendBridge.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

class MTRenderingBackendBridgeImpl : public MTRenderingBackendBridge
{
public:
    explicit MTRenderingBackendBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate* metalDelegate);
    ~MTRenderingBackendBridgeImpl();

    bool setup() override;
    bool initialize() override;
    bool update() override;
    bool render() override;
    bool terminate() override;

    ObjectStatus getStatus() override;

    bool resize() override;
    bool reloadShader(RenderPassType renderPassType) override;
    bool bakeGI() override;

    bool initializeMTMeshDataComponent(MTMeshDataComponent* rhs) override;
    bool initializeMTTextureDataComponent(MTTextureDataComponent* rhs) override;

private:
    ObjectStatus m_objectStatus = ObjectStatus::Terminated;
    MacWindowDelegate* m_macWindowDelegate = nullptr;
    MetalDelegate* m_metalDelegate = nullptr;
};

#endif /* MTRenderingBackendBridgeImpl_h */
