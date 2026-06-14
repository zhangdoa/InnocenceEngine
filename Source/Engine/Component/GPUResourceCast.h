#pragma once
// Out-of-line definitions of GPUResourceComponent::As<T>() / TryAs<T>().
// Separate from the base header because the downcast needs the derived types.
// Include where a checked downcast is instantiated.
#include "GPUResourceComponent.h"
#include "TextureComponent.h"
#include "GPUBufferComponent.h"
#include "SamplerComponent.h"
#include "../Common/LogService.h"
#include "../Common/LogServiceSpecialization.h"
#include "../Engine.h"

namespace Inno
{
	// static_cast is correct (concrete types publicly derive the base); the
	// discriminator check guards its precondition.
	template<typename T>
	inline T* GPUResourceComponent::As()
	{
		if (m_GPUResourceType != T::GetResourceType())
		{
			Log(Error, "GPUResourceComponent::As: '", m_InstanceName,
				"' is not a ", T::GetTypeName(), "; caller declared a mismatched type.");
			return nullptr;
		}
		return static_cast<T*>(this);
	}

	template<typename T>
	inline T* GPUResourceComponent::TryAs()
	{
		if (m_GPUResourceType != T::GetResourceType())
			return nullptr;
		return static_cast<T*>(this);
	}
}
