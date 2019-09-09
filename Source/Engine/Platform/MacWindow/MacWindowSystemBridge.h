#ifndef MacWindowSystemBridge_h
#define MacWindowSystemBridge_h

#include "../../Common/InnoType.h"

class MacWindowSystemBridge
{
public:
  MacWindowSystemBridge() {};
  virtual ~MacWindowSystemBridge() {};

  virtual bool setup(uint32_t sizeX, uint32_t sizeY) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;
};

#endif /* MacWindowSystemBridge_h */
