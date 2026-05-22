#pragma once
#include "../../Common/Array.h"
#include "../../Common/MathHelper.h"
#include "../../Common/EntityID.h"
#include "../../Common/GraphicsPrimitive.h"

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
		bool createPxMesh(uint64_t index, Vec4 position, Vec4 rotation, Vec4 scale, bool isDynamic, bool isConvex, Inno::Array<Vertex>& vertices, Inno::Array<Index>& indices);

		bool addForce(EntityID Entity, Vec4 force);

		bool OnSceneUnloading();

	private:
		PhysXWrapper() {};
	};
}
