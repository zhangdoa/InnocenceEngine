#pragma once
#include "../Common/MathHelper.h"

namespace Inno
{
	enum class LightType { Directional, Point, Spot, Sphere, Disk, Tube, Rectangle };

	struct LightComponent
	{
		static uint32_t GetTypeID() { return 3; };
		static const char* GetTypeName() { return "LightComponent"; };

		// CIE 1931 RGB, clamped [0, 1].
		Vec4 m_RGBColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

		// Per-LightType shape parameters, units in meters (unused for Directional):
		//   Point      x = attenuation radius (auto-calculated)
		//   Spot       x = cut-off angle
		//   Sphere     x = sphere radius
		//   Disk       x = disk radius
		//   Tube       x = length, y = radius
		//   Rectangle  x = width,  y = height
		Vec4 m_Shape = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

		LightType m_LightType = LightType::Directional;

		// Kelvin
		float m_ColorTemperature = 5780.0f;

		// Lumen
		float m_LuminousFlux = 1.0f;

		bool m_UseColorTemperature = true;

		// Honoured only for LightType::Point and LightType::Sphere; Directional
		// always shadows via SunShadowRTPass and ignores this field.
		bool m_CastShadow = true;
	};
}