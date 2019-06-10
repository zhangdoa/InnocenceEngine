//
//  MacWindowSystemBridgeImpl.h
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MacWindowSystemBridgeImpl_h
#define MacWindowSystemBridgeImpl_h

#import "../../../System/MacWindow/MacWindowSystemBridge.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

class MacWindowSystemBridgeImpl : public MacWindowSystemBridge
{
public:
    explicit MacWindowSystemBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate* metalDelegate);
    ~MacWindowSystemBridgeImpl();
    
    bool setup(unsigned int sizeX, unsigned int sizeY) override;
    bool initialize() override;
    bool update() override;
    bool terminate() override;
    
    ObjectStatus getStatus() override;
private:
    ObjectStatus m_objectStatus = ObjectStatus::Terminated;
    MacWindowDelegate* m_macWindowDelegate = nullptr;
    MetalDelegate* m_metalDelegate = nullptr;
    NSApplication *app;
};

#endif /* MacWindowSystemBridgeImpl_h */
