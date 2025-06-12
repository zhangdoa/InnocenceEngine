#pragma once
#include "../Common/Object.h"
#include "../Common/MathHelper.h"

namespace Inno
{
	enum class LightType { Directional, Point, Spot, Sphere, Disk, Tube, Rectangle };

	class LightComponent : public Component
	{
	public:
		static uint32_t GetTypeID() { return 3; };
		static const char* GetTypeName() { return "LightComponent"; };

		Transform m_Transform = {};

		// Unitless: use clamped range from 0.0 to 1.0
		// CIE 1931 RGB color space
		Vec4 m_RGBColor = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

		// Unit: Meter (m)
		// For Directional light it's useless
		// For Point light, x is the auto-calculated attenuation radius
		// For Spot light, x is the cut-off angle
		// For Sphere light, x is the sphere radius
		// For Disk light, x is the disk radius
		// For Tube light, x is the tube length, y is the tube radius
		// For Rectangle light, x is the width, y is the height
		Vec4 m_Shape = Vec4(1.0f, 1.0f, 1.0f, 1.0f);

		LightType m_LightType = LightType::Directional;

		// Unit: Kelvin (K)
		float m_ColorTemperature = 5780.0f;

		// Unit: Lumen (lm)
		float m_LuminousFlux = 1.0f;

		bool m_UseColorTemperature = true;

		std::vector<AABB> m_LitRegion_WorldSpace;
		std::vector<AABB> m_LitRegion_LightSpace;
		std::vector<Mat4> m_ViewMatrices;
		std::vector<Mat4> m_ProjectionMatrices;
	};
}