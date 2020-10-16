#pragma once
#include "ISystem.h"

class IRenderingClient : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IRenderingClient);

	virtual bool Render() = 0;
};