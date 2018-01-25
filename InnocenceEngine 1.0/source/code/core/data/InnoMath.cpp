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

vec3 vec3::operator+(const vec3 & rhs)
{
	return vec3(x + rhs.x, y + rhs.y, z + rhs.z);
}

vec3 vec3::add(float rhs)
{
	return vec3(x + rhs, y + rhs, z + rhs);
}

vec3 vec3::operator+(float rhs)
{
	return vec3(x + rhs, y + rhs, z + rhs);
}

vec3 vec3::sub(const vec3 & rhs)
{
	return vec3(x - rhs.x, y - rhs.y, z - rhs.z);
}

vec3 vec3::operator-(const vec3 & rhs)
{
	return vec3(x - rhs.x, y - rhs.y, z - rhs.z);
}

vec3 vec3::sub(float rhs)
{
	return vec3(x - rhs, y - rhs, z - rhs);
}

vec3 vec3::operator-(float rhs)
{
	return vec3(x - rhs, y - rhs, z - rhs);
}

float vec3::dot(const vec3 & rhs)
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

float vec3::dot(float rhs)
{
	return x * rhs + y * rhs + z * rhs;
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

quat::quat()
{
	x = 0.0f;
	y = 0.0f;
	z = 0.0f;
	w = 1.0f;
}

quat::quat(float rhsX, float rhsY, float rhsZ, float rhsW)
{
	x = rhsX;
	y = rhsY;
	z = rhsZ;
	w = rhsW;
}

quat::quat(const quat & rhs)
{
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
	w = rhs.w;
}

quat & quat::operator=(const quat & rhs)
{
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
	w = rhs.w;
}

quat::~quat()
{
}

quat quat::mul(const quat & rhs)
{
	return quat(
		w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
		w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
		w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
		w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
	);
}

quat quat::mul(float rhs)
{
	return quat(
		w * rhs + x * rhs + y * rhs - z * rhs,
		w * rhs - x * rhs + y * rhs + z * rhs,
		w * rhs + x * rhs - y * rhs + z * rhs,
		w * rhs - x * rhs - y * rhs - z * rhs
	);
}

float quat::length()
{
	// @TODO: replace with override impl
	return sqrtf(x * x + y * y + z * z + w * w);
}

quat quat::normalize()
{	
	// @TODO: optimaze
	return quat(x / length(), y / length(), z / length(), w / length());
}

mat4::mat4()
{
	m[0][0] = 0.0f;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = 0.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 0.0f;
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 0.0f;
}

mat4::mat4(const mat4 & rhs)
{
	m[0][0] = rhs[0][0];
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = 0.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 0.0f;
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 0.0f;
}

mat4 & mat4::operator=(const mat4 & rhs)
{
	// TODO: insert return statement here
}

float* mat4::operator[](int index)
{
	return m[index];
}

mat4 mat4::mul(const mat4 & rhs)
{
	return mat4();
}

mat4 mat4::mul(float rhs)
{
	return mat4();
}
