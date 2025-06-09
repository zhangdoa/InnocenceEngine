#pragma once
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Object.h"

namespace Inno
{
	struct DescriptorHandle
	{
		uint64_t m_CPUHandle = 0;
		uint64_t m_GPUHandle = 0;
		uint32_t m_Index = 0;
	};

	class GPUResourceComponent : public Component
	{
	public:
		GPUResourceType m_GPUResourceType = GPUResourceType::Sampler;
		Accessibility m_CPUAccessibility = Accessibility::WriteOnly;
		Accessibility m_GPUAccessibility = Accessibility::ReadOnly;	
		std::vector<DescriptorHandle> m_ReadHandles;
		std::vector<DescriptorHandle> m_WriteHandles;
	};
}
