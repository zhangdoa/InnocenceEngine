#include "../../main/stdafx.h"
#include "InnoMath.h"

vec3::vec3()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
}

vec3::vec3(float rhsX, float rhsY, float rhsZ)
{
	x = rhsX;
	y = rhsY;
	z = rhsZ;
}

vec3::vec3(const vec3 & rhs)
{
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
}

vec3& vec3::operator=(const vec3 & rhs)
{
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
	return *this;
}

vec3::~vec3()
{
}

vec3 vec3::add(const vec3 & rhs)
{
	return vec3(x + rhs.x, y + rhs.y, z + rhs.z);
}

vec3 vec3::sub(const vec3 & rhs)
{
	return vec3(x - rhs.x, y - rhs.y, z - rhs.z);
}

float vec3::dot(const vec3 & rhs)
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

vec3 vec3::cross(const vec3 & rhs)
{
	return vec3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
}

float vec3::length()
{
	// @TODO: replace with override impl
	return sqrtf(x * x + y * y + z * z);
}

vec3 vec3::normalize()
{
	// @TODO: optimaze
	return vec3(x / length(), y / length(), z / length());
}

vec2::vec2()
{
	x = 0.0f;
	y = 0.0f;
}

vec2::vec2(float rhsX, float rhsY)
{
	x = rhsX;
	y = rhsY;
}

vec2::vec2(const vec2 & rhs)
{
	x = rhs.x;
	y = rhs.y;
}

vec2& vec2::operator=(const vec2 & rhs)
{
	x = rhs.x;
	y = rhs.y;
	return *this;
}

vec2::~vec2()
{
}

vec2 vec2::add(const vec2 & rhs)
{
	return vec2(x + rhs.x, y + rhs.y);
}

vec2 vec2::sub(const vec2 & rhs)
{
	return vec2(x - rhs.x, y - rhs.y);
}

float vec2::dot(const vec2 & rhs)
{
	return x * rhs.x + y * rhs.y;
}

float vec2::length()
{
	return sqrtf(x * x + y * y);
}

vec2 vec2::normalize()
{
	return vec2(x / length(), y / length());
}
