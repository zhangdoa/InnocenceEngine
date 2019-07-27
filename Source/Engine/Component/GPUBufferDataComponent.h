#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoComponent.h"

enum class GPUBufferAccessibility { ReadOnly = 1, WriteOnly = 2, ReadWrite = ReadOnly | WriteOnly };

class GPUBufferDataComponent : public InnoComponent
{
public:
	GPUBufferAccessibility m_GPUBufferAccessibility = GPUBufferAccessibility::ReadOnly;
	size_t m_ElementCount = 0;
	size_t m_ElementSize = 0;
	size_t m_TotalSize = 0;
	size_t m_BindingPoint = 0;
	void* m_InitialData = 0;
};
