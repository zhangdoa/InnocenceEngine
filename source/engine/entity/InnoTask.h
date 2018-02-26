#pragma once
#include "common/stdafx.h"
class InnoTask
{
public:
	InnoTask();
	~InnoTask();

private:
	std::function<void()> m_functor;
};

