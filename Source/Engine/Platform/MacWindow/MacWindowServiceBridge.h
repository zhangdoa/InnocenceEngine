#ifndef MacWindowServiceBridge_h
#define MacWindowServiceBridge_h

namespace Inno
{
	class MacWindowServiceBridge
	{
	public:
		MacWindowServiceBridge() {};
		virtual ~MacWindowServiceBridge() {};

		virtual bool Setup(uint32_t sizeX, uint32_t sizeY) = 0;
		virtual bool Initialize() = 0;
		virtual bool Update() = 0;
		virtual bool Terminate() = 0;

		virtual ObjectStatus GetStatus() = 0;
	};
}
#endif /* MacWindowServiceBridge_h */
