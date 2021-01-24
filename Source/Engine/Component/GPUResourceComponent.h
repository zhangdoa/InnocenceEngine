#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoObject.h"

namespace Inno
{
	class GPUResourceComponent : public InnoComponent
	{
	public:
		GPUResourceType m_GPUResourceType = GPUResourceType::Sampler;
		Accessibility m_CPUAccessibility = Accessibility::WriteOnly;
		Accessibility m_GPUAccessibility = Accessibility::ReadOnly;
	};
}
