#pragma once
#include "interface/IGraphicPrimitive.h"

class IRenderPass : public IGraphicPrimitive
{
public:
	IRenderPass() {};
	virtual ~IRenderPass() {};

protected:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};
