#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoMath.h"

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

	bool createPxActor(void* component, vec4 globalPos, vec4 size);
private:
	PhysXWrapper() {};
};
