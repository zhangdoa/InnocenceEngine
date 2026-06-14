#pragma once
#include "../Common/Array.h"
#include "../Common/GraphicsPrimitive.h"
#include "../Common/Object.h"

namespace Inno
{
	struct GPUResourceComponent
	{
		GPUResourceType   m_GPUResourceType  = GPUResourceType::Sampler;
		Accessibility     m_CPUAccessibility = Accessibility::WriteOnly;
		Accessibility     m_GPUAccessibility = Accessibility::ReadOnly;
		uint32_t          m_ReadState        = 0;
		uint32_t          m_WriteState       = 0;
		Inno::Array<DescriptorHandle> m_ReadHandles;
		Inno::Array<DescriptorHandle> m_WriteHandles;
		ObjectStatus      m_ObjectStatus     = ObjectStatus::Invalid;
		ObjectName        m_InstanceName     = "";

		// Checked downcasts to the concrete type, validating m_GPUResourceType.
		// As<T>() logs+nullptr on mismatch; TryAs<T>() is the silent probe.
		// Definitions in GPUResourceCast.h (needs the derived types).
		template<typename T> T* As();
		template<typename T> T* TryAs();
	};
}
