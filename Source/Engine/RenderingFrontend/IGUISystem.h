#pragma once
#include "../Interface/ISystem.h"

class IGUISystem : public ISystem
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGUISystem);

	virtual bool Render() = 0;
};