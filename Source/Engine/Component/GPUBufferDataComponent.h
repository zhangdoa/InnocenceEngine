#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoGraphicsPrimitive.h"
#include "../Common/InnoComponent.h"

class GPUBufferDataComponent : public InnoComponent
{
public:
	Accessibility m_CPUAccessibility = Accessibility::WriteOnly;
	Accessibility m_GPUAccessibility = Accessibility::ReadOnly;
	size_t m_ElementCount = 0;
	size_t m_ElementSize = 0;
	size_t m_TotalSize = 0;
	bool m_isAtomicCounter = false;
	void* m_InitialData = 0;
	IResourceBinder* m_ResourceBinder = 0;
};
