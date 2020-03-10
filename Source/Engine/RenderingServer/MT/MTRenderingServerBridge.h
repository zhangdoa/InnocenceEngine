#ifndef MTRenderingServerBridge_h
#define MTRenderingServerBridge_h

#include "../../Common/InnoType.h"
#include "../../Component/MTMeshDataComponent.h"
#include "../../Component/MTTextureDataComponent.h"

class MTRenderingServerBridge
{
public:
  MTRenderingServerBridge() {};
  virtual ~MTRenderingServerBridge() {};

  virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
  virtual	bool render() = 0;
  virtual	bool present() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool resize() = 0;

  virtual bool initializeMTMeshDataComponent(MTMeshDataComponent* rhs) = 0;
  virtual bool initializeMTTextureDataComponent(MTTextureDataComponent* rhs) = 0;
};

#endif /* MTRenderingSystemBridge_h */
