#ifndef MTRenderingSystemBridge_h
#define MTRenderingSystemBridge_h

#include "../../../Common/InnoType.h"
#include "../../../Component/MTMeshDataComponent.h"
#include "../../../Component/MTTextureDataComponent.h"

class MTRenderingBackendBridge
{
public:
  MTRenderingBackendBridge() {};
  virtual ~MTRenderingBackendBridge() {};

  virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
  virtual	bool render() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual bool resize() = 0;
	virtual bool reloadShader(RenderPassType renderPassType) = 0;
	virtual bool bakeGI() = 0;

  virtual bool initializeMTMeshDataComponent(MTMeshDataComponent* rhs) = 0;
  virtual bool initializeMTTextureDataComponent(MTTextureDataComponent* rhs) = 0;
};

#endif /* MTRenderingSystemBridge_h */
