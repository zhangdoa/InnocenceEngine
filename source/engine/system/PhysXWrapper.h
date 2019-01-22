#pragma once
#include "../common/InnoType.h"

class PhysXWrapper
{
public:
	~PhysXWrapper() {};

	static PhysXWrapper& get()
	{
		static PhysXWrapper instance;
		return instance;
	}
	void setup();
	void initialize();
	void update();
	void terminate();

private:
	PhysXWrapper() {};
};