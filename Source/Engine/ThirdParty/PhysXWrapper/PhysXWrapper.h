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

		bool createPxSphere(uint64_t index, const TransformVector& transformVector, float radius, bool isDynamic);
		bool createPxBox(uint64_t index, const TransformVector& transformVector, bool isDynamic);
		bool createPxMesh(uint64_t index, const TransformVector& transformVector, bool isDynamic, bool isConvex, std::vector<Vertex>& vertices, std::vector<Index>& indices);

		bool addForce(CollisionComponent* rhs, Vec4 force);

	private:
		PhysXWrapper() {};
	};
}