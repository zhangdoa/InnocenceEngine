//
//  MTGraphicsServiceBridgeImpl.h
//  Main
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MTGraphicsServiceBridgeImpl_h
#define MTGraphicsServiceBridgeImpl_h

#import "../../Services/MT/MTGraphicsServiceBridge.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

namespace Inno
{
	class MTGraphicsServiceBridgeImpl : public MTGraphicsServiceBridge
	{
	public:
		explicit MTGraphicsServiceBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate* metalDelegate);
		~MTGraphicsServiceBridgeImpl();

		bool Setup() override;
		bool Initialize() override;
		bool Update() override;
		bool render() override;
		bool present() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool resize() override;

		bool initializeMTMeshComponent(MTMeshComponent* rhs) override;
		bool initializeMTTextureComponent(MTTextureComponent* rhs) override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		MacWindowDelegate* m_macWindowDelegate = nullptr;
		MetalDelegate* m_metalDelegate = nullptr;
	};
}
#endif /* MTGraphicsServiceBridgeImpl_h */
