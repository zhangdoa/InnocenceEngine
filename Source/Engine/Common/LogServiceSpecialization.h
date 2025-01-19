#include "LogService.h"
#include "MathHelper.h"
#include "Object.h"

namespace Inno
{
	template <>
	inline void LogService::LogContent(const Vec2& values)
	{
		LogContent("Vec2(x: ", values.x, ", y: ", values.y, ")");
	}

	template <>
	inline void LogService::LogContent(const Vec4& values)
	{
		LogContent("Vec4(x: ", values.x, ", y: ", values.y, ", z: ", values.z, ", w: ", values.w, ")");
	}

	template <>
	inline void LogService::LogContent(const Mat4& values)
	{
		LogContent(
			"Mat4: \n|",
			values.m00, "", values.m10, "", values.m20, "", values.m30, "|\n|",
			values.m01, "", values.m11, "", values.m21, "", values.m31, "|\n|",
			values.m02, "", values.m12, "", values.m22, "", values.m32, "|\n|",
			values.m03, "", values.m13, "", values.m23, "", values.m33, "|\n");
	}

	template <>
	inline void LogService::LogContent(ObjectName& values)
	{
		LogContent("[Object Name: ", values.c_str(), "]");
	}

	template <>
	inline void LogService::LogContent(const ObjectName& values)
	{
		LogContent(const_cast<ObjectName&>(values));
	}
}