#pragma once
#include "common/stdafx.h"
#include "IManager.h"

class IRenderingManager : public IManager
{
public:
	virtual ~IRenderingManager() {};

	virtual void render() = 0;
};

IRenderingManager* g_pRenderingManager;