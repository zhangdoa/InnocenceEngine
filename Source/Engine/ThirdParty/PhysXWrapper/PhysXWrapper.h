#pragma once
#include "../../Common/InnoMathHelper.h"

class PhysXWrapper
{
public:
	~PhysXWrapper() {};

	static PhysXWrapper& get()
	{
		static PhysXWrapper instance;
		return instance;
	}
	bool setup();
	bool initialize();
	bool update();
	bool terminate();

	bool createPxSphere(void* component, Vec4 globalPos, float radius, bool isDynamic);
	bool createPxBox(void* component, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic);

private:
	PhysXWrapper() {};
};
