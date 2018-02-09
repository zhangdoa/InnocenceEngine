#pragma once
#include "../interface/IApplication.h"
class BaseApplication : public IApplication
{
public:
	virtual void setup() override;
	virtual void initialize() override;
	virtual void update() override;
	virtual void shutdown() override;
};

