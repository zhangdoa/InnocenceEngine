#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
	struct RigidBodyComponent
	{
		Vec3  m_LinearVelocity  = {};
		Vec3  m_AngularVelocity = {};
		float m_Mass            = 1.f;
		void* m_SimulationProxy = nullptr;
	};
}
