#ifndef MacWindowSystemBridge_h
#define MacWindowSystemBridge_h

#include "../../common/InnoType.h"

class MacWindowSystemBridge
{
public:
  MacWindowSystemBridge() {};
  virtual ~MacWindowSystemBridge() {};

  virtual bool setup(unsigned int sizeX, unsigned int sizeY) = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;
};

#endif /* MacWindowSystemBridge_h */
