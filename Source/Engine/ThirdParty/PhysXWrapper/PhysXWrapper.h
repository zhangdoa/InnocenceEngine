#pragma once
#include "../../Common/MathHelper.h"
#include "../../Component/ModelComponent.h"

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

		bool createPxSphere(uint64_t index, Vec4 position, float radius, bool isDynamic);
		bool createPxBox(uint64_t index, Vec4 position, Vec4 rotation, Vec4 scale, bool isDynamic);
		bool createPxMesh(uint64_t index, Vec4 position, Vec4 rotation, Vec4 scale, bool isDynamic, bool isConvex, std::vector<Vertex>& vertices, std::vector<Index>& indices);

		// Updated to use ModelComponent instead of CollisionComponent
		bool addForce(ModelComponent* rhs, Vec4 force);

	private:
		PhysXWrapper() {};
	};
}
