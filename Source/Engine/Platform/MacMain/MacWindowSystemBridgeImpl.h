//
//  MacWindowSystemBridgeImpl.h
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MacWindowSystemBridgeImpl_h
#define MacWindowSystemBridgeImpl_h

#import "../MacWindow/MacWindowSystemBridge.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

class MacWindowSystemBridgeImpl : public MacWindowSystemBridge
{
public:
    explicit MacWindowSystemBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate* metalDelegate);
    ~MacWindowSystemBridgeImpl();

    bool setup(uint32_t sizeX, uint32_t sizeY) override;
    bool initialize() override;
    bool update() override;
    bool terminate() override;

    ObjectStatus getStatus() override;
private:
    ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
    MacWindowDelegate* m_macWindowDelegate = nullptr;
    MetalDelegate* m_metalDelegate = nullptr;
    NSApplication *app;
};

#endif /* MacWindowSystemBridgeImpl_h */
