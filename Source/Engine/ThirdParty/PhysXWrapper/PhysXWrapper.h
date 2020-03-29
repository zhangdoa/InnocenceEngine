#pragma once
#include "../../Common/InnoMathHelper.h"
#include "../../Component/PhysicsDataComponent.h"

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

	bool createPxSphere(PhysicsDataComponent* rhs, Vec4 globalPos, float radius, bool isDynamic);
	bool createPxBox(PhysicsDataComponent* rhs, Vec4 globalPos, Vec4 rot, Vec4 size, bool isDynamic);

	bool addForce(PhysicsDataComponent* rhs, Vec4 force);

private:
	PhysXWrapper() {};
};
