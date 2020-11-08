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

namespace Inno
{
	class MacWindowSystemBridgeImpl : public MacWindowSystemBridge
	{
	public:
		explicit MacWindowSystemBridgeImpl(MacWindowDelegate *macWindowDelegate, MetalDelegate *metalDelegate);
		~MacWindowSystemBridgeImpl();

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
#endif /* MacWindowSystemBridgeImpl_h */
