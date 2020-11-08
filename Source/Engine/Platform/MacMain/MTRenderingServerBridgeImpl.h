//
//  MTRenderingServerBridgeImpl.h
//  InnoMain
//
//  Created by zhangdoa on 14/04/2019.
//  Copyright © 2019 InnocenceEngine. All rights reserved.
//

#ifndef MTRenderingServerBridgeImpl_h
#define MTRenderingServerBridgeImpl_h

#import "../../RenderingServer/MT/MTRenderingServerBridge.h"
#import "MacWindowDelegate.h"
#import "MetalDelegate.h"

namespace Inno
{
	class MTRenderingServerBridgeImpl : public MTRenderingServerBridge
	{
	public:
		explicit MTRenderingServerBridgeImpl(MacWindowDelegate* macWindowDelegate, MetalDelegate* metalDelegate);
		~MTRenderingServerBridgeImpl();

		bool Setup() override;
		bool Initialize() override;
		bool Update() override;
		bool render() override;
		bool present() override;
		bool Terminate() override;

		ObjectStatus GetStatus() override;

		bool resize() override;

		bool initializeMTMeshDataComponent(MTMeshDataComponent* rhs) override;
		bool initializeMTTextureDataComponent(MTTextureDataComponent* rhs) override;

	private:
		ObjectStatus m_ObjectStatus = ObjectStatus::Terminated;
		MacWindowDelegate* m_macWindowDelegate = nullptr;
		MetalDelegate* m_metalDelegate = nullptr;
	};
}
#endif /* MTRenderingServerBridgeImpl_h */
