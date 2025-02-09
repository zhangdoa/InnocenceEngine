#pragma once
#include "../../Common/MathHelper.h"
#include "../../Component/CollisionComponent.h"

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

		bool createPxSphere(CollisionComponent* rhs, float radius, bool isDynamic);
		bool createPxBox(CollisionComponent* rhs, bool isDynamic);
		bool createPxMesh(CollisionComponent* rhs, bool isDynamic, bool isConvex);

		bool addForce(CollisionComponent* rhs, Vec4 force);

	private:
		PhysXWrapper() {};
	};
}