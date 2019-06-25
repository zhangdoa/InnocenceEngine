#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoMath.h"

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

	bool createPxSphere(void* component, vec4 globalPos, float radius);
	bool createPxBox(void* component, vec4 globalPos, vec4 rot, vec4 size);

private:
	PhysXWrapper() {};
};
