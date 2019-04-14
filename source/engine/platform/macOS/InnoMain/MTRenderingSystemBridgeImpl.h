//
//  MTRenderingSystemBridgeImpl.h
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MTRenderingSystemBridgeImpl_h
#define MTRenderingSystemBridgeImpl_h

#import "../../../system/MTRenderingBackend/MTRenderingSystemBridge.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

class MTRenderingSystemBridgeImpl : public MTRenderingSystemBridge
{
public:
    explicit MTRenderingSystemBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate* metalDelegate);
    ~MTRenderingSystemBridgeImpl();
    
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
    MacWindowDelegate* m_macWindowDelegate = nullptr;
    MetalDelegate* m_metalDelegate = nullptr;
};

#endif /* MTRenderingSystemBridgeImpl_h */
