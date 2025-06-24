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
		uint32_t m_ReadState = 0;
		uint32_t m_WriteState = 0;
		std::vector<DescriptorHandle> m_ReadHandles;
		std::vector<DescriptorHandle> m_WriteHandles;
	};
}
