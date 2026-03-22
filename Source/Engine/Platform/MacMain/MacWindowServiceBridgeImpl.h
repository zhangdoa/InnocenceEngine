//
//  MacWindowServiceBridgeImpl.h
//  Main
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MacWindowServiceBridgeImpl_h
#define MacWindowServiceBridgeImpl_h

#import "../MacWindow/MacWindowServiceBridge.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

namespace Inno
{
	class MacWindowServiceBridgeImpl : public MacWindowServiceBridge
	{
	public:
		explicit MacWindowServiceBridgeImpl(MacWindowDelegate *macWindowDelegate, MetalDelegate *metalDelegate);
		~MacWindowServiceBridgeImpl();

		bool Setup(uint32_t sizeX, uint32_t sizeY) override;
		bool Initialize() override;
		bool Update() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		MacWindowDelegate *m_macWindowDelegate = nullptr;
		MetalDelegate *m_metalDelegate = nullptr;
		NSApplication *app;
	};
} // namespace Inno
#endif /* MacWindowServiceBridgeImpl_h */
