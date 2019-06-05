#ifndef MTRenderingSystemBridge_h
#define MTRenderingSystemBridge_h

#include "../../common/InnoType.h"
#include "../../component/MTMeshDataComponent.h"
#include "../../component/MTTextureDataComponent.h"

class MTRenderingSystemBridge
{
public:
  MTRenderingSystemBridge() {};
  virtual ~MTRenderingSystemBridge() {};

  virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool resize() = 0;
	virtual bool reloadShader(RenderPassType renderPassType) = 0;
	virtual bool bakeGI() = 0;

  virtual bool initializeMTMeshDataComponent(MTMeshDataComponent* rhs) = 0;
  virtual bool initializeMTTextureDataComponent(MTTextureDataComponent* rhs) = 0;
};

#endif /* MTRenderingSystemBridge_h */
