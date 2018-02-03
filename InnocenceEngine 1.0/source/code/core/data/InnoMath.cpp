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

float vec3::operator*(const vec3 & rhs)
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

vec3 vec3::cross(const vec3 & rhs)
{
	return vec3(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
}

vec3 vec3::mul(const vec3 & rhs)
{
	return vec3(x * rhs.x, y * rhs.y, z * rhs.z);
}

vec3 vec3::mul(float rhs)
{
	return vec3(x * rhs, y * rhs, z * rhs);
}

vec3 vec3::operator*(float rhs)
{
	return vec3(x * rhs, y * rhs, z * rhs);
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

bool vec3::operator!=(const vec3 & rhs)
{
	//@TODO: optimize
	if (x != rhs.x)
	{
		return true;
	}
	else if (y != rhs.y)
	{
		return true;
	}
	else if (z != rhs.z)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool vec3::operator==(const vec3 & rhs)
{
	//@TODO: optimize
	return !(*this != rhs);
}

mat4 vec3::toTranslationMartix()
{
	mat4 l_m;

	l_m.m[0][0] = 1;
	l_m.m[1][1] = 1;
	l_m.m[2][2] = 1;
	l_m.m[3][3] = 1;

	l_m.m[3][0] = x;
	l_m.m[3][1] = y;
	l_m.m[3][2] = z;

	return l_m;
}

mat4 vec3::toScaleMartix()
{
	mat4 l_m;
	l_m.m[0][0] = x;
	l_m.m[1][1] = y;
	l_m.m[2][2] = z;
	l_m.m[3][3] = 1;
	return l_m;
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

	return *this;
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

bool quat::operator!=(const quat & rhs)
{
	//@TODO: optimize
	if (x != rhs.x)
	{
		return true;
	}
	else if (y != rhs.y)
	{
		return true;
	}
	else if (z != rhs.z)
	{
		return true;
	}
	else if (w != rhs.w)
	{
		return true;
	}
	else
	{
		return false;
	}
}

bool quat::operator==(const quat & rhs)
{
	//@TODO: optimize
	return !(*this != rhs);
}

mat4 quat::toRotationMartix()
{
	//@TODO:optimize
	mat4 l_m;

	l_m.m[0][0] = 1 - 2 * y * y - 2 * z * z;
	l_m.m[1][0] = 2 * x * y - 2 * z * w;
	l_m.m[2][0] = 2 * x * z + 2 * y * w;
	l_m.m[3][0] = 0;

	l_m.m[0][1] = 2 * x * y + 2 * z * w;
	l_m.m[1][1] = 1 - 2 * x * x - 2 * z * z;
	l_m.m[2][1] = 2 * y * z - 2 * x * w;
	l_m.m[3][1] = 0;

	l_m.m[0][2] = 2 * x * z - 2 * y * w;
	l_m.m[1][2] = 2 * y * z + 2 * x * w;
	l_m.m[2][2] = 1 - 2 * x * x - 2 * y * y;
	l_m.m[3][2] = 0;

	l_m.m[0][3] = 0;
	l_m.m[1][3] = 0;
	l_m.m[2][3] = 0;
	l_m.m[3][3] = 1;

	return l_m;
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
	m[0][0] = rhs.m[0][0];
	m[0][1] = rhs.m[0][1];
	m[0][2] = rhs.m[0][2];
	m[0][3] = rhs.m[0][3];
	m[1][0] = rhs.m[1][0];
	m[1][1] = rhs.m[1][1];
	m[1][2] = rhs.m[1][2];
	m[1][3] = rhs.m[1][3];
	m[2][0] = rhs.m[2][0];
	m[2][1] = rhs.m[2][1];
	m[2][2] = rhs.m[2][2];
	m[2][3] = rhs.m[2][3];
	m[3][0] = rhs.m[3][0];
	m[3][1] = rhs.m[3][1];
	m[3][2] = rhs.m[3][2];
	m[3][3] = rhs.m[3][3];
}

mat4 & mat4::operator=(const mat4 & rhs)
{
	m[0][0] = rhs.m[0][0];
	m[0][1] = rhs.m[0][1];
	m[0][2] = rhs.m[0][2];
	m[0][3] = rhs.m[0][3];
	m[1][0] = rhs.m[1][0];
	m[1][1] = rhs.m[1][1];
	m[1][2] = rhs.m[1][2];
	m[1][3] = rhs.m[1][3];
	m[2][0] = rhs.m[2][0];
	m[2][1] = rhs.m[2][1];
	m[2][2] = rhs.m[2][2];
	m[2][3] = rhs.m[2][3];
	m[3][0] = rhs.m[3][0];
	m[3][1] = rhs.m[3][1];
	m[3][2] = rhs.m[3][2];
	m[3][3] = rhs.m[3][3];

	return *this;
}

mat4 mat4::operator*(const mat4 & rhs)
{
	//@TODO:optimize
	mat4 l_m;

	l_m.m[0][0] = m[0][0] * rhs.m[0][0] + m[1][0] * rhs.m[0][1] + m[2][0] * rhs.m[0][2] + m[3][0] * rhs.m[0][3];
	l_m.m[0][1] = m[0][1] * rhs.m[0][0] + m[1][1] * rhs.m[0][1] + m[2][1] * rhs.m[0][2] + m[3][1] * rhs.m[0][3];
	l_m.m[0][2] = m[0][2] * rhs.m[0][0] + m[1][2] * rhs.m[0][1] + m[2][2] * rhs.m[0][2] + m[3][2] * rhs.m[0][3];
	l_m.m[0][3] = m[0][3] * rhs.m[0][0] + m[1][3] * rhs.m[0][1] + m[2][3] * rhs.m[0][2] + m[3][3] * rhs.m[0][3];

	l_m.m[1][0] = m[0][0] * rhs.m[1][0] + m[1][0] * rhs.m[1][1] + m[2][0] * rhs.m[1][2] + m[3][0] * rhs.m[1][3];
	l_m.m[1][1] = m[0][1] * rhs.m[1][0] + m[1][1] * rhs.m[1][1] + m[2][1] * rhs.m[1][2] + m[3][1] * rhs.m[1][3];
	l_m.m[1][2] = m[0][2] * rhs.m[1][0] + m[1][2] * rhs.m[1][1] + m[2][2] * rhs.m[1][2] + m[3][2] * rhs.m[1][3];
	l_m.m[1][3] = m[0][3] * rhs.m[1][0] + m[1][3] * rhs.m[1][1] + m[2][3] * rhs.m[1][2] + m[3][3] * rhs.m[1][3];

	l_m.m[2][0] = m[0][0] * rhs.m[2][0] + m[1][0] * rhs.m[2][1] + m[2][0] * rhs.m[2][2] + m[3][0] * rhs.m[2][3];
	l_m.m[2][1] = m[0][1] * rhs.m[2][0] + m[1][1] * rhs.m[2][1] + m[2][1] * rhs.m[2][2] + m[3][1] * rhs.m[2][3];
	l_m.m[2][2] = m[0][2] * rhs.m[2][0] + m[1][2] * rhs.m[2][1] + m[2][2] * rhs.m[2][2] + m[3][2] * rhs.m[2][3];
	l_m.m[2][3] = m[0][3] * rhs.m[2][0] + m[1][3] * rhs.m[2][1] + m[2][3] * rhs.m[2][2] + m[3][3] * rhs.m[2][3];

	l_m.m[3][0] = m[0][0] * rhs.m[3][0] + m[1][0] * rhs.m[3][1] + m[2][0] * rhs.m[3][2] + m[3][0] * rhs.m[3][3];
	l_m.m[3][1] = m[0][1] * rhs.m[3][0] + m[1][1] * rhs.m[3][1] + m[2][1] * rhs.m[3][2] + m[3][1] * rhs.m[3][3];
	l_m.m[3][2] = m[0][2] * rhs.m[3][0] + m[1][2] * rhs.m[3][1] + m[2][2] * rhs.m[3][2] + m[3][2] * rhs.m[3][3];
	l_m.m[3][3] = m[0][3] * rhs.m[3][0] + m[1][3] * rhs.m[3][1] + m[2][3] * rhs.m[3][2] + m[3][3] * rhs.m[3][3];

	return l_m;
}

mat4 mat4::operator*(float rhs)
{
	//@TODO:optimize
	mat4 l_m;

	l_m.m[0][0] = rhs * m[0][0];
	l_m.m[0][1] = rhs * m[0][1];
	l_m.m[0][2] = rhs * m[0][2];
	l_m.m[0][3] = rhs * m[0][3];
	l_m.m[1][0] = rhs * m[1][0];
	l_m.m[1][1] = rhs * m[1][1];
	l_m.m[1][2] = rhs * m[1][2];
	l_m.m[1][3] = rhs * m[1][3];
	l_m.m[2][0] = rhs * m[2][0];
	l_m.m[2][1] = rhs * m[2][1];
	l_m.m[2][2] = rhs * m[2][2];
	l_m.m[2][3] = rhs * m[2][3];
	l_m.m[3][0] = rhs * m[3][0];
	l_m.m[3][1] = rhs * m[3][1];
	l_m.m[3][2] = rhs * m[3][2];
	l_m.m[3][3] = rhs * m[3][3];

	return l_m;
}

mat4 mat4::mul(const mat4 & rhs)
{
	//@TODO:optimize
	mat4 l_m;

	l_m.m[0][0] = m[0][0] * rhs.m[0][0] + m[1][0] * rhs.m[0][1] + m[2][0] * rhs.m[0][2] + m[3][0] * rhs.m[0][3];
	l_m.m[0][1] = m[0][1] * rhs.m[0][0] + m[1][1] * rhs.m[0][1] + m[2][1] * rhs.m[0][2] + m[3][1] * rhs.m[0][3];
	l_m.m[0][2] = m[0][2] * rhs.m[0][0] + m[1][2] * rhs.m[0][1] + m[2][2] * rhs.m[0][2] + m[3][2] * rhs.m[0][3];
	l_m.m[0][3] = m[0][3] * rhs.m[0][0] + m[1][3] * rhs.m[0][1] + m[2][3] * rhs.m[0][2] + m[3][3] * rhs.m[0][3];

	l_m.m[1][0] = m[0][0] * rhs.m[1][0] + m[1][0] * rhs.m[1][1] + m[2][0] * rhs.m[1][2] + m[3][0] * rhs.m[1][3];
	l_m.m[1][1] = m[0][1] * rhs.m[1][0] + m[1][1] * rhs.m[1][1] + m[2][1] * rhs.m[1][2] + m[3][1] * rhs.m[1][3];
	l_m.m[1][2] = m[0][2] * rhs.m[1][0] + m[1][2] * rhs.m[1][1] + m[2][2] * rhs.m[1][2] + m[3][2] * rhs.m[1][3];
	l_m.m[1][3] = m[0][3] * rhs.m[1][0] + m[1][3] * rhs.m[1][1] + m[2][3] * rhs.m[1][2] + m[3][3] * rhs.m[1][3];

	l_m.m[2][0] = m[0][0] * rhs.m[2][0] + m[1][0] * rhs.m[2][1] + m[2][0] * rhs.m[2][2] + m[3][0] * rhs.m[2][3];
	l_m.m[2][1] = m[0][1] * rhs.m[2][0] + m[1][1] * rhs.m[2][1] + m[2][1] * rhs.m[2][2] + m[3][1] * rhs.m[2][3];
	l_m.m[2][2] = m[0][2] * rhs.m[2][0] + m[1][2] * rhs.m[2][1] + m[2][2] * rhs.m[2][2] + m[3][2] * rhs.m[2][3];
	l_m.m[2][3] = m[0][3] * rhs.m[2][0] + m[1][3] * rhs.m[2][1] + m[2][3] * rhs.m[2][2] + m[3][3] * rhs.m[2][3];

	l_m.m[3][0] = m[0][0] * rhs.m[3][0] + m[1][0] * rhs.m[3][1] + m[2][0] * rhs.m[3][2] + m[3][0] * rhs.m[3][3];
	l_m.m[3][1] = m[0][1] * rhs.m[3][0] + m[1][1] * rhs.m[3][1] + m[2][1] * rhs.m[3][2] + m[3][1] * rhs.m[3][3];
	l_m.m[3][2] = m[0][2] * rhs.m[3][0] + m[1][2] * rhs.m[3][1] + m[2][2] * rhs.m[3][2] + m[3][2] * rhs.m[3][3];
	l_m.m[3][3] = m[0][3] * rhs.m[3][0] + m[1][3] * rhs.m[3][1] + m[2][3] * rhs.m[3][2] + m[3][3] * rhs.m[3][3];

	return l_m;
}

mat4 mat4::mul(float rhs)
{
	//@TODO:optimize
	mat4 l_m;

	l_m.m[0][0] = rhs * m[0][0];
	l_m.m[0][1] = rhs * m[0][1];
	l_m.m[0][2] = rhs * m[0][2];
	l_m.m[0][3] = rhs * m[0][3];
	l_m.m[1][0] = rhs * m[1][0];
	l_m.m[1][1] = rhs * m[1][1];
	l_m.m[1][2] = rhs * m[1][2];
	l_m.m[1][3] = rhs * m[1][3];
	l_m.m[2][0] = rhs * m[2][0];
	l_m.m[2][1] = rhs * m[2][1];
	l_m.m[2][2] = rhs * m[2][2];
	l_m.m[2][3] = rhs * m[2][3];
	l_m.m[3][0] = rhs * m[3][0];
	l_m.m[3][1] = rhs * m[3][1];
	l_m.m[3][2] = rhs * m[3][2];
	l_m.m[3][3] = rhs * m[3][3];

	return l_m;
}

void mat4::initializeToPerspectiveMatrix(float FOV, float HWRatio, float zNear, float zFar)
{
	m[0][0] = 1 / (tanf(FOV / 2) * HWRatio);
	m[1][1] = 1 / tanf(FOV / 2);
	m[2][2] = -(zFar + zNear) / ((zFar - zNear));
	m[3][2] = -(2 * zFar *zNear) / ((zFar - zNear));
	m[2][3] = -1;
}

mat4 mat4::lookAt(const vec3 & eyePos, const vec3 & centerPos, const vec3 & upDir)
{
	mat4 l_m;
	vec3 l_X;
	vec3 l_Y;
	vec3 l_Z = eyePos;

	l_Z = l_Z - centerPos;
	l_Z = l_Z.normalize();

	l_Y = upDir;

	l_X = l_Y.cross(l_Z);

	l_Y = l_Z.cross(l_X);

	l_X = l_X.normalize();
	l_Y = l_Y.normalize();

	l_m.m[0][0] = l_X.x;
	l_m.m[1][0] = l_X.y;
	l_m.m[2][0] = l_X.z;
	l_m.m[3][0] = -l_X.dot(eyePos);
	l_m.m[0][1] = l_Y.x;
	l_m.m[1][1] = l_Y.y;
	l_m.m[2][1] = l_Y.z;
	l_m.m[3][1] = -l_Y.dot(eyePos);
	l_m.m[0][2] = l_Z.x;
	l_m.m[1][2] = l_Z.y;
	l_m.m[2][2] = l_Z.z;
	l_m.m[3][2] = -l_Z.dot(eyePos);
	l_m.m[0][3] = 0;
	l_m.m[1][3] = 0;
	l_m.m[2][3] = 0;
	l_m.m[3][3] = 1.0f;
	
	return l_m;
}
