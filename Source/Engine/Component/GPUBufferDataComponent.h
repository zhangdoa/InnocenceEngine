#pragma once
#include "../Common/InnoType.h"
#include "../Common/InnoComponent.h"

enum class GPUBufferAccessibility { ReadOnly = 1, WriteOnly = 2, ReadWrite = ReadOnly | WriteOnly };

class GPUBufferDataComponent : public InnoComponent
{
public:
	GPUBufferDataComponent() {};
	~GPUBufferDataComponent() {};

	GPUBufferAccessibility m_GPUBufferAccessibility = GPUBufferAccessibility::ReadOnly;
	size_t m_Size = 0;
	size_t m_BindingPoint = 0;
};
