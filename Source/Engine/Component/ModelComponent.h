#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"
#include "../Common/GPUDataStructure.h"
#include "MeshComponent.h"
#include "SkeletonComponent.h"
#include "MaterialComponent.h"

namespace Inno
{
	class DrawCallComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 200; }
		static const char* GetTypeName() { return "DrawCallComponent"; }

		uint64_t m_MeshComponent = 0;
		uint64_t m_MaterialComponent = 0;
		uint64_t m_SkeletonComponent = 0;
	};

	class ModelComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 2; }
		static const char* GetTypeName() { return "ModelComponent"; }

		Transform m_Transform;
		
		CollisionPrimitives m_CollisionPrimitives = {};
		void* m_SimulationProxy = 0;

		AABB& m_AABB = m_CollisionPrimitives.m_AABB;

		std::vector<uint64_t> m_DrawCallComponents;
	};
}
