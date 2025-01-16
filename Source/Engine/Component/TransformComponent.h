#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"
#include "../Interface/ISystem.h"

namespace Inno
{
	class TransformComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 1; };
		static const char* GetTypeName() { return "TransformComponent"; };

		TransformVector m_localTransformVector;
		TransformVector m_localTransformVector_target;
		TransformVector m_globalTransformVector;
		TransformMatrix m_globalTransformMatrix;

		TransformMatrix m_globalTransformMatrix_prev;

		uint32_t m_transformHierarchyLevel = 0;
		TransformComponent* m_parentTransformComponent = 0;
	};

	class ITransformSystem : public ISystem
	{
	public:
		INNO_CLASS_INTERFACE_NON_COPYABLE(ITransformSystem);

		virtual const TransformComponent* GetRootTransformComponent() = 0;
	};
}
