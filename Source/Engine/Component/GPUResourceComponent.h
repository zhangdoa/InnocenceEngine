#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Object.h"

namespace Inno
{
	class GPUResourceComponent : public Component
	{
	public:
		GPUResourceType m_GPUResourceType = GPUResourceType::Sampler;
		Accessibility m_CPUAccessibility = Accessibility::WriteOnly;
		Accessibility m_GPUAccessibility = Accessibility::ReadOnly;
	};
}
