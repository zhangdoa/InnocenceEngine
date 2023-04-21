#ifndef MacWindowSystemBridge_h
#define MacWindowSystemBridge_h

#include "../../Common/Type.h"

namespace Inno
{
	class MacWindowSystemBridge
	{
	public:
		MacWindowSystemBridge() {};
		virtual ~MacWindowSystemBridge() {};

		virtual bool Setup(uint32_t sizeX, uint32_t sizeY) = 0;
		virtual bool Initialize() = 0;
		virtual bool Update() = 0;
		virtual bool Terminate() = 0;

		virtual ObjectStatus GetStatus() = 0;
	};
}
#endif /* MacWindowSystemBridge_h */
