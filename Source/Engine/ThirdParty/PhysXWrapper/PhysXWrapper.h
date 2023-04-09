#pragma once
#include "../../Common/InnoMathHelper.h"
#include "../../Component/PhysicsComponent.h"

namespace Inno
{
	class PhysXWrapper
	{
	public:
		~PhysXWrapper() {};

		static PhysXWrapper& get()
		{
			static PhysXWrapper instance;
			return instance;
		}
		bool Setup();
		bool Initialize();
		bool Update();
		bool Terminate();

		bool createPxSphere(PhysicsComponent* rhs, float radius, bool isDynamic);
		bool createPxBox(PhysicsComponent* rhs, bool isDynamic);
		bool createPxMesh(PhysicsComponent* rhs, bool isDynamic, bool isConvex);

		bool addForce(PhysicsComponent* rhs, Vec4 force);

	private:
		PhysXWrapper() {};
	};
}