#include "InnoMath.h"

vec3::vec3()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;
}

vec3::vec3(double rhsX, double rhsY, double rhsZ)
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

vec3 vec3::add(double rhs)
{
	return vec3(x + rhs, y + rhs, z + rhs);
}

vec3 vec3::operator+(double rhs)
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

vec3 vec3::sub(double rhs)
{
	return vec3(x - rhs, y - rhs, z - rhs);
}

vec3 vec3::operator-(double rhs)
{
	return vec3(x - rhs, y - rhs, z - rhs);
}

double vec3::dot(const vec3 & rhs)
{
	return x * rhs.x + y * rhs.y + z * rhs.z;
}

double vec3::operator*(const vec3 & rhs)
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

vec3 vec3::mul(double rhs)
{
	return vec3(x * rhs, y * rhs, z * rhs);
}

vec3 vec3::operator*(double rhs)
{
	return vec3(x * rhs, y * rhs, z * rhs);
}

double vec3::length()
{
	// @TODO: replace with SIMD impl
	return sqrt(x * x + y * y + z * z);
}

vec3 vec3::normalize()
{
	// @TODO: replace with SIMD impl
	return vec3(x / length(), y / length(), z / length());
}

bool vec3::operator!=(const vec3 & rhs)
{
	// @TODO: replace with SIMD impl
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
	// @TODO: replace with SIMD impl
	return !(*this != rhs);
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 vec3::toTranslationMartix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)1;
	l_m.m[1][1] = (float)1;
	l_m.m[2][2] = (float)1;
	l_m.m[3][0] = (float)x;
	l_m.m[3][1] = (float)y;
	l_m.m[3][2] = (float)z;
	l_m.m[3][3] = (float)1;

	return l_m;
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
mat4 vec3::toTranslationMartix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)1;
	l_m.m[0][3] = (float)x;
	l_m.m[1][1] = (float)1;
	l_m.m[1][3] = (float)y;
	l_m.m[2][2] = (float)1;
	l_m.m[2][3] = (float)z;
	l_m.m[3][3] = (float)1;

	return l_m;
}
#endif

mat4 vec3::toScaleMartix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;
	l_m.m[0][0] = (float)x;
	l_m.m[1][1] = (float)y;
	l_m.m[2][2] = (float)z;
	l_m.m[3][3] = (float)1;
	return l_m;
}

vec2::vec2()
{
	x = 0.0;
	y = 0.0;
}

vec2::vec2(double rhsX, double rhsY)
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

vec2 vec2::operator+(const vec2 & rhs)
{
	return vec2(x + rhs.x, y + rhs.y);
}

vec2 vec2::add(double rhs)
{
	return vec2(x + rhs, y + rhs);
}

vec2 vec2::operator+(double rhs)
{
	return vec2(x + rhs, y + rhs);
}

vec2 vec2::sub(const vec2 & rhs)
{
	return vec2(x - rhs.x, y - rhs.y);
}

vec2 vec2::operator-(const vec2 & rhs)
{
	return vec2(x - rhs.x, y - rhs.y);
}

vec2 vec2::sub(double rhs)
{
	return vec2(x - rhs, y - rhs);
}

vec2 vec2::operator-(double rhs)
{
	return vec2(x - rhs, y - rhs);
}

double vec2::dot(const vec2 & rhs)
{
	return x * rhs.x + y * rhs.y;
}

double vec2::operator*(const vec2 & rhs)
{
	return x * rhs.x + y * rhs.y;
}

vec2 vec2::mul(const vec2 & rhs)
{
	return vec2(x * rhs.x, y * rhs.y);
}

vec2 vec2::mul(double rhs)
{
	return vec2(x * rhs, y * rhs);
}

vec2 vec2::operator*(double rhs)
{
	return vec2(x * rhs, y * rhs);
}

double vec2::length()
{
	return sqrt(x * x + y * y);
}

vec2 vec2::normalize()
{
	return vec2(x / length(), y / length());
}

quat::quat()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;
	w = 1.0;
}

quat::quat(double rhsX, double rhsY, double rhsZ, double rhsW)
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

quat quat::mul(double rhs)
{
	return quat(
		w * rhs + x * rhs + y * rhs - z * rhs,
		w * rhs - x * rhs + y * rhs + z * rhs,
		w * rhs + x * rhs - y * rhs + z * rhs,
		w * rhs - x * rhs - y * rhs - z * rhs
	);
}

double quat::length()
{
	// @TODO: replace with override impl
	return sqrt(x * x + y * y + z * z + w * w);
}

quat quat::normalize()
{	
	// @TODO: replace with SIMD impl
	return quat(x / length(), y / length(), z / length(), w / length());
}

bool quat::operator!=(const quat & rhs)
{
	// @TODO: replace with SIMD impl
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
	// @TODO: replace with SIMD impl
	return !(*this != rhs);
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 quat::toRotationMartix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)(1 - 2 * y * y - 2 * z * z);
	l_m.m[0][1] = (float)(2 * x * y + 2 * z * w);
	l_m.m[0][2] = (float)(2 * x * z - 2 * y * w);
	l_m.m[0][3] = (float)(0);

	l_m.m[1][0] = (float)(2 * x * y - 2 * z * w);
	l_m.m[1][1] = (float)(1 - 2 * x * x - 2 * z * z);
	l_m.m[1][2] = (float)(2 * y * z + 2 * x * w);
	l_m.m[1][3] = (float)(0);

	l_m.m[2][0] = (float)(2 * x * z + 2 * y * w);
	l_m.m[2][1] = (float)(2 * y * z - 2 * x * w);
	l_m.m[2][2] = (float)(1 - 2 * x * x - 2 * y * y);
	l_m.m[2][3] = (float)(0);

	l_m.m[3][0] = (float)(0);
	l_m.m[3][1] = (float)(0);	
	l_m.m[3][2] = (float)(0);
	l_m.m[3][3] = (float)(1);

	return l_m;
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
mat4 quat::toRotationMartix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)(1 - 2 * y * y - 2 * z * z);
	l_m.m[0][1] = (float)(2 * x * y - 2 * z * w);
	l_m.m[0][2] = (float)(2 * x * z + 2 * y * w);
	l_m.m[0][3] = (float)(0);

	l_m.m[1][0] = (float)(2 * x * y + 2 * z * w);
	l_m.m[1][1] = (float)(1 - 2 * x * x - 2 * z * z);
	l_m.m[1][2] = (float)(2 * y * z - 2 * x * w);
	l_m.m[1][3] = (float)(0);

	l_m.m[2][0] = (float)(2 * x * z - 2 * y * w);
	l_m.m[2][1] = (float)(2 * y * z + 2 * x * w);
	l_m.m[2][2] = (float)(1 - 2 * x * x - 2 * y * y);
	l_m.m[2][3] = (float)(0);

	l_m.m[3][0] = (float)(0);
	l_m.m[3][1] = (float)(0);
	l_m.m[3][2] = (float)(0);
	l_m.m[3][3] = (float)(1);

	return l_m;
}
#endif

mat4::mat4()
{
	m[0][0] = 0.0;
	m[0][1] = 0.0;
	m[0][2] = 0.0;
	m[0][3] = 0.0;
	m[1][0] = 0.0;
	m[1][1] = 0.0;
	m[1][2] = 0.0;
	m[1][3] = 0.0;
	m[2][0] = 0.0;
	m[2][1] = 0.0;
	m[2][2] = 0.0;
	m[2][3] = 0.0;
	m[3][0] = 0.0;
	m[3][1] = 0.0;
	m[3][2] = 0.0;
	m[3][3] = 0.0;
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

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 mat4::operator*(const mat4 & rhs)
{
	// @TODO: replace with SIMD impl
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
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
mat4 mat4::operator*(const mat4 & rhs)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = m[0][0] * rhs.m[0][0] + m[0][1] * rhs.m[1][0] + m[0][2] * rhs.m[2][0] + m[0][3] * rhs.m[3][0];
	l_m.m[0][1] = m[1][0] * rhs.m[0][0] + m[1][1] * rhs.m[1][0] + m[1][2] * rhs.m[2][0] + m[1][3] * rhs.m[3][0];
	l_m.m[0][2] = m[2][0] * rhs.m[0][0] + m[2][1] * rhs.m[1][0] + m[2][2] * rhs.m[2][0] + m[2][3] * rhs.m[3][0];
	l_m.m[0][3] = m[3][0] * rhs.m[0][0] + m[3][1] * rhs.m[1][0] + m[3][2] * rhs.m[2][0] + m[3][3] * rhs.m[3][0];

	l_m.m[0][0] = m[0][0] * rhs.m[0][1] + m[0][1] * rhs.m[1][1] + m[0][2] * rhs.m[2][1] + m[0][3] * rhs.m[3][1];
	l_m.m[1][1] = m[1][0] * rhs.m[0][1] + m[1][1] * rhs.m[1][1] + m[1][2] * rhs.m[2][1] + m[1][3] * rhs.m[3][1];
	l_m.m[1][2] = m[2][0] * rhs.m[0][1] + m[2][1] * rhs.m[1][1] + m[2][2] * rhs.m[2][1] + m[2][3] * rhs.m[3][1];
	l_m.m[1][3] = m[3][0] * rhs.m[0][1] + m[3][1] * rhs.m[1][1] + m[3][2] * rhs.m[2][1] + m[3][3] * rhs.m[3][1];

	l_m.m[0][0] = m[0][0] * rhs.m[0][2] + m[0][1] * rhs.m[1][2] + m[0][2] * rhs.m[2][2] + m[0][3] * rhs.m[3][2];
	l_m.m[2][1] = m[1][0] * rhs.m[0][2] + m[1][1] * rhs.m[1][2] + m[1][2] * rhs.m[2][2] + m[1][3] * rhs.m[3][2];
	l_m.m[2][2] = m[2][0] * rhs.m[0][2] + m[2][1] * rhs.m[1][2] + m[2][2] * rhs.m[2][2] + m[2][3] * rhs.m[3][2];
	l_m.m[2][3] = m[3][0] * rhs.m[0][2] + m[3][1] * rhs.m[1][2] + m[3][2] * rhs.m[2][2] + m[3][3] * rhs.m[3][2];

	l_m.m[3][0] = m[0][0] * rhs.m[0][3] + m[0][1] * rhs.m[1][3] + m[0][2] * rhs.m[2][3] + m[0][3] * rhs.m[3][3];
	l_m.m[3][1] = m[1][0] * rhs.m[0][3] + m[1][1] * rhs.m[1][3] + m[1][2] * rhs.m[2][3] + m[1][3] * rhs.m[3][3];
	l_m.m[3][2] = m[2][0] * rhs.m[0][3] + m[2][1] * rhs.m[1][3] + m[2][2] * rhs.m[2][3] + m[2][3] * rhs.m[3][3];
	l_m.m[3][3] = m[3][0] * rhs.m[0][3] + m[3][1] * rhs.m[1][3] + m[3][2] * rhs.m[2][3] + m[3][3] * rhs.m[3][3];

	return l_m;
}
#endif 

mat4 mat4::operator*(double rhs)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)rhs * m[0][0];
	l_m.m[0][1] = (float)rhs * m[0][1];
	l_m.m[0][2] = (float)rhs * m[0][2];
	l_m.m[0][3] = (float)rhs * m[0][3];
	l_m.m[1][0] = (float)rhs * m[1][0];
	l_m.m[1][1] = (float)rhs * m[1][1];
	l_m.m[1][2] = (float)rhs * m[1][2];
	l_m.m[1][3] = (float)rhs * m[1][3];
	l_m.m[2][0] = (float)rhs * m[2][0];
	l_m.m[2][1] = (float)rhs * m[2][1];
	l_m.m[2][2] = (float)rhs * m[2][2];
	l_m.m[2][3] = (float)rhs * m[2][3];
	l_m.m[3][0] = (float)rhs * m[3][0];
	l_m.m[3][1] = (float)rhs * m[3][1];
	l_m.m[3][2] = (float)rhs * m[3][2];
	l_m.m[3][3] = (float)rhs * m[3][3];

	return l_m;
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 mat4::mul(const mat4 & rhs)
{
	// @TODO: replace with SIMD impl
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
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
mat4 mat4::mul(const mat4 & rhs)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = m[0][0] * rhs.m[0][0] + m[0][1] * rhs.m[1][0] + m[0][2] * rhs.m[2][0] + m[0][3] * rhs.m[3][0];
	l_m.m[0][1] = m[1][0] * rhs.m[0][0] + m[1][1] * rhs.m[1][0] + m[1][2] * rhs.m[2][0] + m[1][3] * rhs.m[3][0];
	l_m.m[0][2] = m[2][0] * rhs.m[0][0] + m[2][1] * rhs.m[1][0] + m[2][2] * rhs.m[2][0] + m[2][3] * rhs.m[3][0];
	l_m.m[0][3] = m[3][0] * rhs.m[0][0] + m[3][1] * rhs.m[1][0] + m[3][2] * rhs.m[2][0] + m[3][3] * rhs.m[3][0];

	l_m.m[0][0] = m[0][0] * rhs.m[0][1] + m[0][1] * rhs.m[1][1] + m[0][2] * rhs.m[2][1] + m[0][3] * rhs.m[3][1];
	l_m.m[1][1] = m[1][0] * rhs.m[0][1] + m[1][1] * rhs.m[1][1] + m[1][2] * rhs.m[2][1] + m[1][3] * rhs.m[3][1];
	l_m.m[1][2] = m[2][0] * rhs.m[0][1] + m[2][1] * rhs.m[1][1] + m[2][2] * rhs.m[2][1] + m[2][3] * rhs.m[3][1];
	l_m.m[1][3] = m[3][0] * rhs.m[0][1] + m[3][1] * rhs.m[1][1] + m[3][2] * rhs.m[2][1] + m[3][3] * rhs.m[3][1];

	l_m.m[0][0] = m[0][0] * rhs.m[0][2] + m[0][1] * rhs.m[1][2] + m[0][2] * rhs.m[2][2] + m[0][3] * rhs.m[3][2];
	l_m.m[2][1] = m[1][0] * rhs.m[0][2] + m[1][1] * rhs.m[1][2] + m[1][2] * rhs.m[2][2] + m[1][3] * rhs.m[3][2];
	l_m.m[2][2] = m[2][0] * rhs.m[0][2] + m[2][1] * rhs.m[1][2] + m[2][2] * rhs.m[2][2] + m[2][3] * rhs.m[3][2];
	l_m.m[2][3] = m[3][0] * rhs.m[0][2] + m[3][1] * rhs.m[1][2] + m[3][2] * rhs.m[2][2] + m[3][3] * rhs.m[3][2];

	l_m.m[3][0] = m[0][0] * rhs.m[0][3] + m[0][1] * rhs.m[1][3] + m[0][2] * rhs.m[2][3] + m[0][3] * rhs.m[3][3];
	l_m.m[3][1] = m[1][0] * rhs.m[0][3] + m[1][1] * rhs.m[1][3] + m[1][2] * rhs.m[2][3] + m[1][3] * rhs.m[3][3];
	l_m.m[3][2] = m[2][0] * rhs.m[0][3] + m[2][1] * rhs.m[1][3] + m[2][2] * rhs.m[2][3] + m[2][3] * rhs.m[3][3];
	l_m.m[3][3] = m[3][0] * rhs.m[0][3] + m[3][1] * rhs.m[1][3] + m[3][2] * rhs.m[2][3] + m[3][3] * rhs.m[3][3];

	return l_m;
}
#endif

mat4 mat4::mul(double rhs)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)rhs * m[0][0];
	l_m.m[0][1] = (float)rhs * m[0][1];
	l_m.m[0][2] = (float)rhs * m[0][2];
	l_m.m[0][3] = (float)rhs * m[0][3];
	l_m.m[1][0] = (float)rhs * m[1][0];
	l_m.m[1][1] = (float)rhs * m[1][1];
	l_m.m[1][2] = (float)rhs * m[1][2];
	l_m.m[1][3] = (float)rhs * m[1][3];
	l_m.m[2][0] = (float)rhs * m[2][0];
	l_m.m[2][1] = (float)rhs * m[2][1];
	l_m.m[2][2] = (float)rhs * m[2][2];
	l_m.m[2][3] = (float)rhs * m[2][3];
	l_m.m[3][0] = (float)rhs * m[3][0];
	l_m.m[3][1] = (float)rhs * m[3][1];
	l_m.m[3][2] = (float)rhs * m[3][2];
	l_m.m[3][3] = (float)rhs * m[3][3];

	return l_m;
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double HWRatio, double zNear, double zFar)
{
	m[0][0] = (float)(1 / (tan(FOV / 2) * HWRatio));
	m[1][1] = (float)(1 / tan(FOV / 2));
	m[2][2] = (float)(-(zFar + zNear) / ((zFar - zNear)));
	m[2][3] = -1.0f;
	m[3][2] = (float)(-(2 * zFar *zNear) / ((zFar - zNear)));
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double HWRatio, double zNear, double zFar)
{
	m[0][0] = (float)(1 / (tan(FOV / 2) * HWRatio));
	m[1][1] = (float)(1 / tan(FOV / 2));
	m[2][2] = (float)(-(zFar + zNear) / ((zFar - zNear)));
	m[2][3] = (float)(-(2 * zFar *zNear) / ((zFar - zNear)));
	m[3][2] = -1.0f;
}
#endif

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
void mat4::initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar)
{
	m[0][0] = (float)(2 / (right - left));
	m[1][1] = (float)(2 / (up - bottom));
	m[2][2] = (float)(-2 / (zFar - zNear));
	m[3][0] = (float)(- (left + right) / (left - right));
	m[3][1] = (float)(- (up + bottom) / (up - bottom));
	m[3][2] = (float)(- (zFar + zNear) / (zFar - zNear));
	m[3][3] = 1.0f;
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar)
{
	m[0][0] = (float)(2 / (right - left));
	m[0][3] = (float)(-(left + right) / (left - right));
	m[1][1] = (float)(2 / (up - bottom));
	m[1][3] = (float)(-(up + bottom) / (up - bottom));
	m[2][2] = (float)(-2 / (zFar - zNear));
	m[2][3] = (float)(-(zFar + zNear) / (zFar - zNear));
	m[3][3] = 1.0f;
}
#endif

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 mat4::lookAt(const vec3 & eyePos, const vec3 & centerPos, const vec3 & upDir)
{
	// @TODO: replace with SIMD impl
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

	l_m.m[0][0] = (float)l_X.x;
	l_m.m[0][1] = (float)l_Y.x;
	l_m.m[0][2] = (float)l_Z.x;
	l_m.m[0][3] = 0.0f;
	l_m.m[1][0] = (float)l_X.y;
	l_m.m[1][1] = (float)l_Y.y;
	l_m.m[1][2] = (float)l_Z.y;
	l_m.m[1][3] = 0.0f;
	l_m.m[2][0] = (float)l_X.z;
	l_m.m[2][1] = (float)l_Y.z;
	l_m.m[2][2] = (float)l_Z.z;
	l_m.m[2][3] = 0.0f;
	l_m.m[3][0] = (float)-l_X.dot(eyePos);
	l_m.m[3][1] = (float)-l_Y.dot(eyePos);
	l_m.m[3][2] = (float)-l_Z.dot(eyePos);
	l_m.m[3][3] = 1.0f;

	return l_m;
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
mat4 mat4::lookAt(const vec3 & eyePos, const vec3 & centerPos, const vec3 & upDir)
{
	// @TODO: replace with SIMD impl
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

	l_m.m[0][0] = (float)l_X.x;
	l_m.m[0][1] = (float)l_X.y;
	l_m.m[0][2] = (float)l_X.z;
	l_m.m[0][3] = (float)-l_X.dot(eyePos);
	l_m.m[1][0] = (float)l_Y.x;
	l_m.m[1][1] = (float)l_Y.y;
	l_m.m[1][2] = (float)l_Y.z;
	l_m.m[1][3] = (float)-l_Y.dot(eyePos);
	l_m.m[2][0] = (float)l_Z.x;
	l_m.m[2][1] = (float)l_Z.y;
	l_m.m[2][2] = (float)l_Z.z;
	l_m.m[2][3] = (float)-l_Z.dot(eyePos);
	l_m.m[3][0] = 0.0f;
	l_m.m[3][1] = 0.0f;
	l_m.m[3][2] = 0.0f;
	l_m.m[3][3] = 1.0f;

	return l_m;
}
#endif

Vertex::Vertex()
{
}

Vertex::Vertex(const Vertex & rhs)
{
	m_pos = rhs.m_pos;
	m_texCoord = rhs.m_texCoord;
	m_normal = rhs.m_normal;
}

Vertex& Vertex::operator=(const Vertex & rhs)
{
	m_pos = rhs.m_pos;
	m_texCoord = rhs.m_texCoord;
	m_normal = rhs.m_normal;
	return *this;
}

Vertex::Vertex(const vec3& pos, const vec2& texCoord, const vec3& normal)
{
	m_pos = pos;
	m_texCoord = texCoord;
	m_normal = normal;
}

Vertex::~Vertex()
{
}

AABB::AABB()
{
	m_center = vec3(0.0, 0.0, 0.0);
	m_halfWidths = vec3(1.0, 0.0, 0.0);
}

AABB::AABB(const AABB & rhs)
{
	m_center = rhs.m_center;
	m_halfWidths = rhs.m_halfWidths;
}

AABB & AABB::operator=(const AABB & rhs)
{
	m_center = rhs.m_center;
	m_halfWidths = rhs.m_halfWidths;
	return *this;
}

AABB::AABB(const vec3 & center, const vec3 & halfWidths)
{
	m_center = center;
	m_halfWidths = halfWidths;
}

AABB::~AABB()
{
}

Transform::Transform()
{
	m_pos = vec3(0.0f, 0.0f, 0.0f);
	m_rot = quat(0.0f, 0.0f, 0.0f, 1.0f);
	m_scale = vec3(1.0f, 1.0f, 1.0f);
	m_oldPos = m_pos + (1.0f);
	m_oldRot = m_rot.mul(0.5f);
	m_oldScale = m_scale + (1.0f);
}

Transform::~Transform()
{
}

void Transform::update()
{
	m_oldPos = m_pos;
	m_oldRot = m_rot;
	m_oldScale = m_scale;
}

void Transform::rotate(const vec3 & axis, double angle)
{
	double sinHalfAngle = sin((angle * PI / 180.0) / 2.0);
	double cosHalfAngle = cos((angle * PI / 180.0) / 2.0);
	// get final rotation
	m_rot = quat(axis.x * sinHalfAngle, axis.y * sinHalfAngle, axis.z * sinHalfAngle, cosHalfAngle).mul(m_rot);
}

vec3 & Transform::getPos()
{
	return m_pos;
}

quat & Transform::getRot()
{
	return m_rot;
}

vec3 & Transform::getScale()
{
	return m_scale;
}

void Transform::setPos(const vec3 & pos)
{
	m_pos = pos;
}

void Transform::setRot(const quat & rot)
{
	m_rot = rot;
}

void Transform::setScale(const vec3 & scale)
{
	m_scale = scale;
}

vec3 & Transform::getOldPos()
{
	return m_oldPos;
}
quat & Transform::getOldRot()
{
	return m_oldRot;
}

vec3 & Transform::getOldScale()
{
	return m_oldScale;
}

vec3 Transform::getDirection(direction direction) const
{
	vec3 l_directionVec3;

	switch (direction)
	{
	case FORWARD: l_directionVec3 = vec3(0.0f, 0.0f, 1.0f); break;
	case BACKWARD:l_directionVec3 = vec3(0.0f, 0.0f, -1.0f); break;
	case UP:l_directionVec3 = vec3(0.0f, 1.0f, 0.0f); break;
	case DOWN:l_directionVec3 = vec3(0.0f, -1.0f, 0.0f); break;
	case RIGHT:l_directionVec3 = vec3(1.0f, 0.0f, 0.0f); break;
	case LEFT:l_directionVec3 = vec3(-1.0f, 0.0f, 0.0f); break;
	}

	// V' = QVQ^-1, for unit quaternion, the conjugated quaternion is as same as the inverse quaternion

	// naive version
	// get Q * V by hand
	//quat l_hiddenRotatedQuat;
	//l_hiddenRotatedQuat.w = -m_rot.x * l_directionVec3.x - m_rot.y * l_directionVec3.y - m_rot.z * l_directionVec3.z;
	//l_hiddenRotatedQuat.x = m_rot.w * l_directionVec3.x + m_rot.y * l_directionVec3.z - m_rot.z * l_directionVec3.y;
	//l_hiddenRotatedQuat.y = m_rot.w * l_directionVec3.y + m_rot.z * l_directionVec3.x - m_rot.x * l_directionVec3.z;
	//l_hiddenRotatedQuat.z = m_rot.w * l_directionVec3.z + m_rot.x * l_directionVec3.y - m_rot.y * l_directionVec3.x;

	// get conjugated quaternion
	//quat l_conjugatedQuat;
	//l_conjugatedQuat = conjugate(m_rot);

	// then QV * Q^-1 
	//quat l_directionQuat;
	//l_directionQuat = l_hiddenRotatedQuat * l_conjugatedQuat;
	//l_directionVec3.x = l_directionQuat.x;
	//l_directionVec3.y = l_directionQuat.y;
	//l_directionVec3.z = l_directionQuat.z;

	// traditional version, change direction vector to quaternion representation

	//quat l_directionQuat = quat(0.0, l_directionVec3);
	//l_directionQuat = m_rot * l_directionQuat * conjugate(m_rot);
	//l_directionVec3.x = l_directionQuat.x;
	//l_directionVec3.y = l_directionQuat.y;
	//l_directionVec3.z = l_directionQuat.z;

	// optimized version ([Kavan et al. ] Lemma 4)
	//V' = V + 2 * Qv x (Qv x V + Qs * V)
	vec3 l_Qv = vec3(m_rot.x, m_rot.y, m_rot.z);
	l_directionVec3 = l_directionVec3 + l_Qv.cross((l_Qv.cross(l_directionVec3) + l_directionVec3.mul(m_rot.w))).mul(2.0f);

	return l_directionVec3;
}

