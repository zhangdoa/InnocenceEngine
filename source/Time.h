#pragma once
#include "stdafx.h"

class Time {
public:

	Time();
	~Time();
	
	const double getTime();

	const double getDelta();


private:

	double delta;

};
