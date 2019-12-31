#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoClassTemplate.h"

class IRenderingClient
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingClient);

	virtual bool Setup() = 0;
	virtual bool Initialize() = 0;
	virtual bool PrepareCommandList() = 0;
	virtual bool ExecuteCommandList() = 0;
	virtual bool Terminate() = 0;
};