#include "InnoMath.h"

vec4::vec4()
{
	x = 0.0;
	y = 0.0;
	z = 0.0;
	w = 1.0;
}

vec4::vec4(double rhsX, double rhsY, double rhsZ, double rhsW)
{
	x = rhsX;
	y = rhsY;
	z = rhsZ;
	w = rhsW;
}

vec4::vec4(const vec4 & rhs)
{
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
	w = rhs.w;
}

vec4& vec4::operator=(const vec4 & rhs)
{
	x = rhs.x;
	y = rhs.y;
	z = rhs.z;
	w = rhs.w;
	return *this;
}

vec4::~vec4()
{
}

vec4 vec4::add(const vec4 & rhs)
{
	return vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

vec4 vec4::operator+(const vec4 & rhs)
{
	return vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

vec4 vec4::add(double rhs)
{
	return vec4(x + rhs, y + rhs, z + rhs, w + rhs);
}

vec4 vec4::operator+(double rhs)
{
	return vec4(x + rhs, y + rhs, z + rhs, w + rhs);
}

vec4 vec4::sub(const vec4 & rhs)
{
	return vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

vec4 vec4::operator-(const vec4 & rhs)
{
	return vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

vec4 vec4::sub(double rhs)
{
	return vec4(x - rhs, y - rhs, z - rhs, w - rhs);
}

vec4 vec4::operator-(double rhs)
{
	return vec4(x - rhs, y - rhs, z - rhs, w - rhs);
}

double vec4::dot(const vec4 & rhs)
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

double vec4::operator*(const vec4 & rhs)
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

vec4 vec4::cross(const vec4 & rhs)
{
	return vec4(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x, 0.0);
}

vec4 vec4::scale(const vec4 & rhs)
{
	return vec4(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
}

vec4 vec4::scale(double rhs)
{
	return vec4(x * rhs, y * rhs, z * rhs, w * rhs);
}

vec4 vec4::operator*(double rhs)
{
	return vec4(x * rhs, y * rhs, z * rhs, w * rhs);
}

vec4 vec4::quatMul(const vec4 & rhs)
{
	return vec4(
		w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
		w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
		w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
		w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
	);
}

vec4 vec4::quatMul(double rhs)
{
	return vec4(
		w * rhs + x * rhs + y * rhs - z * rhs,
		w * rhs - x * rhs + y * rhs + z * rhs,
		w * rhs + x * rhs - y * rhs + z * rhs,
		w * rhs - x * rhs - y * rhs - z * rhs
	);
}

double vec4::length()
{
	// @TODO: replace with SIMD impl
	return sqrt(x * x + y * y + z * z + w * w);
}

vec4 vec4::normalize()
{
	// @TODO: replace with SIMD impl
	auto l_length = length();
	return vec4(x / l_length, y / l_length, z / l_length, w / l_length);
}

bool vec4::operator!=(const vec4 & rhs)
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

bool vec4::operator==(const vec4 & rhs)
{
	// @TODO: replace with SIMD impl
	return !(*this != rhs);
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 vec4::toTranslationMartix()
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
mat4 vec4::toTranslationMartix()
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

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 vec4::toRotationMartix()
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
mat4 vec4::toRotationMartix()
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

mat4 vec4::toScaleMartix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;
	l_m.m[0][0] = (float)x;
	l_m.m[1][1] = (float)y;
	l_m.m[2][2] = (float)z;
	l_m.m[3][3] = (float)w;
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

vec2 vec2::scale(const vec2 & rhs)
{
	return vec2(x * rhs.x, y * rhs.y);
}

vec2 vec2::scale(double rhs)
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
// @TODO: check check check!!!
vec4 mat4::mul(const vec4 & rhs)
{
	// @TODO: replace with SIMD impl
	vec4 l_vec4;

	l_vec4.x = rhs.x * m[0][0] + rhs.y * m[1][0] + rhs.z * m[2][0] + rhs.w * m[3][0];
	l_vec4.y = rhs.x * m[0][1] + rhs.y * m[1][1] + rhs.z * m[2][1] + rhs.w * m[3][1];
	l_vec4.z = rhs.x * m[0][2] + rhs.y * m[1][2] + rhs.z * m[2][2] + rhs.w * m[3][2];
	l_vec4.w = rhs.x * m[0][3] + rhs.y * m[1][3] + rhs.z * m[2][3] + rhs.w * m[3][3];

	return l_vec4;
}

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

mat4 mat4::transpose()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = m[0][0];
	l_m.m[0][1] = m[1][0];
	l_m.m[0][2] = m[2][0];
	l_m.m[0][3] = m[3][0];
	l_m.m[1][0] = m[0][1];
	l_m.m[1][1] = m[1][1];
	l_m.m[1][2] = m[2][1];
	l_m.m[1][3] = m[3][1];
	l_m.m[2][0] = m[0][2];
	l_m.m[2][1] = m[1][2];
	l_m.m[2][2] = m[2][2];
	l_m.m[2][3] = m[3][2];
	l_m.m[3][0] = m[0][3];
	l_m.m[3][1] = m[1][3];
	l_m.m[3][2] = m[2][3];
	l_m.m[3][3] = m[3][3];

	return l_m;
}

mat4 mat4::inverse()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = m[2][1]*m[3][2]*m[1][3] - m[3][1]*m[2][2]*m[1][3] + m[3][1]*m[1][2]*m[2][3] - m[1][1]*m[3][2]*m[2][3] - m[2][1]*m[1][2]*m[3][3] + m[1][1]*m[2][2]*m[3][3];
	l_m.m[1][0] = m[3][0]*m[2][2]*m[1][3] - m[2][0]*m[3][2]*m[1][3] - m[3][0]*m[1][2]*m[2][3] + m[1][0]*m[3][2]*m[2][3] + m[2][0]*m[1][2]*m[3][3] - m[1][0]*m[2][2]*m[3][3];
	l_m.m[2][0] = m[2][0]*m[3][1]*m[1][3] - m[3][0]*m[2][1]*m[1][3] + m[3][0]*m[1][1]*m[2][3] - m[1][0]*m[3][1]*m[2][3] - m[2][0]*m[1][1]*m[3][3] + m[1][0]*m[2][1]*m[3][3];
	l_m.m[3][0] = m[3][0]*m[2][1]*m[1][2] - m[2][0]*m[3][1]*m[1][2] - m[3][0]*m[1][1]*m[2][2] + m[1][0]*m[3][1]*m[2][2] + m[2][0]*m[1][1]*m[3][2] - m[1][0]*m[2][1]*m[3][2];
	l_m.m[0][1] = m[3][1]*m[2][2]*m[0][3] - m[2][1]*m[3][2]*m[0][3] - m[3][1]*m[0][2]*m[2][3] + m[0][1]*m[3][2]*m[2][3] + m[2][1]*m[0][2]*m[3][3] - m[0][1]*m[2][2]*m[3][3];
	l_m.m[1][1] = m[2][0]*m[3][2]*m[0][3] - m[3][0]*m[2][2]*m[0][3] + m[3][0]*m[0][2]*m[2][3] - m[0][0]*m[3][2]*m[2][3] - m[2][0]*m[0][2]*m[3][3] + m[0][0]*m[2][2]*m[3][3];
	l_m.m[2][1] = m[3][0]*m[2][1]*m[0][3] - m[2][0]*m[3][1]*m[0][3] - m[3][0]*m[0][1]*m[2][3] + m[0][0]*m[3][1]*m[2][3] + m[2][0]*m[0][1]*m[3][3] - m[0][0]*m[2][1]*m[3][3];
	l_m.m[3][1] = m[2][0]*m[3][1]*m[0][2] - m[3][0]*m[2][1]*m[0][2] + m[3][0]*m[0][1]*m[2][2] - m[0][0]*m[3][1]*m[2][2] - m[2][0]*m[0][1]*m[3][2] + m[0][0]*m[2][1]*m[3][2];
	l_m.m[0][2] = m[1][1]*m[3][2]*m[0][3] - m[3][1]*m[1][2]*m[0][3] + m[3][1]*m[0][2]*m[1][3] - m[0][1]*m[3][2]*m[1][3] - m[1][1]*m[0][2]*m[3][3] + m[0][1]*m[1][2]*m[3][3];
	l_m.m[1][2] = m[3][0]*m[1][2]*m[0][3] - m[1][0]*m[3][2]*m[0][3] - m[3][0]*m[0][2]*m[1][3] + m[0][0]*m[3][2]*m[1][3] + m[1][0]*m[0][2]*m[3][3] - m[0][0]*m[1][2]*m[3][3];
	l_m.m[2][2] = m[1][0]*m[3][1]*m[0][3] - m[3][0]*m[1][1]*m[0][3] + m[3][0]*m[0][1]*m[1][3] - m[0][0]*m[3][1]*m[1][3] - m[1][0]*m[0][1]*m[3][3] + m[0][0]*m[1][1]*m[3][3];
	l_m.m[3][2] = m[3][0]*m[1][1]*m[0][2] - m[1][0]*m[3][1]*m[0][2] - m[3][0]*m[0][1]*m[1][2] + m[0][0]*m[3][1]*m[1][2] + m[1][0]*m[0][1]*m[3][2] - m[0][0]*m[1][1]*m[3][2];
	l_m.m[0][3] = m[2][1]*m[1][2]*m[0][3] - m[1][1]*m[2][2]*m[0][3] - m[2][1]*m[0][2]*m[1][3] + m[0][1]*m[2][2]*m[1][3] + m[1][1]*m[0][2]*m[2][3] - m[0][1]*m[1][2]*m[2][3];
	l_m.m[1][3] = m[1][0]*m[2][2]*m[0][3] - m[2][0]*m[1][2]*m[0][3] + m[2][0]*m[0][2]*m[1][3] - m[0][0]*m[2][2]*m[1][3] - m[1][0]*m[0][2]*m[2][3] + m[0][0]*m[1][2]*m[2][3];
	l_m.m[2][3] = m[2][0]*m[1][1]*m[0][3] - m[1][0]*m[2][1]*m[0][3] - m[2][0]*m[0][1]*m[1][3] + m[0][0]*m[2][1]*m[1][3] + m[1][0]*m[0][1]*m[2][3] - m[0][0]*m[1][1]*m[2][3];
	l_m.m[3][3] = m[1][0]*m[2][1]*m[0][2] - m[2][0]*m[1][1]*m[0][2] + m[2][0]*m[0][1]*m[1][2] - m[0][0]*m[2][1]*m[1][2] - m[1][0]*m[0][1]*m[2][2] + m[0][0]*m[1][1]*m[2][2];

	l_m = l_m * (1 / this->getDeterminant());
	return l_m;
}

double mat4::getDeterminant()
{
	double value;

	value =
		m[3][0] * m[2][1] * m[1][2] * m[0][3] - m[2][0] * m[3][1] * m[1][2] * m[0][3] - m[3][0] * m[1][1] * m[2][2] * m[0][3] + m[1][0] * m[3][1] * m[2][2] * m[0][3] +
		m[2][0] * m[1][1] * m[3][2] * m[0][3] - m[1][0] * m[2][1] * m[3][2] * m[0][3] - m[3][0] * m[2][1] * m[0][2] * m[1][3] + m[2][0] * m[3][1] * m[0][2] * m[1][3] +
		m[3][0] * m[0][1] * m[2][2] * m[1][3] - m[0][0] * m[3][1] * m[2][2] * m[1][3] - m[2][0] * m[0][1] * m[3][2] * m[1][3] + m[0][0] * m[2][1] * m[3][2] * m[1][3] +
		m[3][0] * m[1][1] * m[0][2] * m[2][3] - m[1][0] * m[3][1] * m[0][2] * m[2][3] - m[3][0] * m[0][1] * m[1][2] * m[2][3] + m[0][0] * m[3][1] * m[1][2] * m[2][3] +
		m[1][0] * m[0][1] * m[3][2] * m[2][3] - m[0][0] * m[1][1] * m[3][2] * m[2][3] - m[2][0] * m[1][1] * m[0][2] * m[3][3] + m[1][0] * m[2][1] * m[0][2] * m[3][3] +
		m[2][0] * m[0][1] * m[1][2] * m[3][3] - m[0][0] * m[2][1] * m[1][2] * m[3][3] - m[1][0] * m[0][1] * m[2][2] * m[3][3] + m[0][0] * m[1][1] * m[2][2] * m[3][3];
	
	return value;
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double HWRatio, double zNear, double zFar)
{
	m[0][0] = (float)(1.0 / (tan(FOV / 2.0) * HWRatio));
	m[1][1] = (float)(1.0 / tan(FOV / 2.0));
	m[2][2] = (float)(-(zFar + zNear) / ((zFar - zNear)));
	m[2][3] = (float)-1.0;
	m[3][2] = (float)(-(2.0 * zFar * zNear) / ((zFar - zNear)));
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double HWRatio, double zNear, double zFar)
{
	m[0][0] = (float)(1.0 / (tan(FOV / 2.0) * HWRatio));
	m[1][1] = (float)(1.0 / tan(FOV / 2.0));
	m[2][2] = (float)(-(zFar + zNear) / ((zFar - zNear)));
	m[2][3] = (float)(-(2.0 * zFar * zNear) / ((zFar - zNear)));
	m[3][2] = (float)-1.0;
}
#endif

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
void mat4::initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar)
{
	m[0][0] = (float)(2.0 / (right - left));
	m[1][1] = (float)(2.0 / (up - bottom));
	m[2][2] = (float)(-2.0 / (zFar - zNear));
	m[3][0] = (float)(- (left + right) / (left - right));
	m[3][1] = (float)(- (up + bottom) / (up - bottom));
	m[3][2] = (float)(- (zFar + zNear) / (zFar - zNear));
	m[3][3] = (float)1.0;
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar)
{
	m[0][0] = (float)(2.0 / (right - left));
	m[0][3] = (float)(-(left + right) / (left - right));
	m[1][1] = (float)(2.0 / (up - bottom));
	m[1][3] = (float)(-(up + bottom) / (up - bottom));
	m[2][2] = (float)(-2.0 / (zFar - zNear));
	m[2][3] = (float)(-(zFar + zNear) / (zFar - zNear));
	m[3][3] = (float)1.0;
}
#endif

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 mat4::lookAt(const vec4 & eyePos, const vec4 & centerPos, const vec4 & upDir)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;
	vec4 l_X;
	vec4 l_Y;
	vec4 l_Z = eyePos;

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
mat4 mat4::lookAt(const vec4 & eyePos, const vec4 & centerPos, const vec4 & upDir)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;
	vec4 l_X;
	vec4 l_Y;
	vec4 l_Z = eyePos;

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

Vertex::Vertex(const vec4& pos, const vec2& texCoord, const vec4& normal)
{
	m_pos = pos;
	m_texCoord = texCoord;
	m_normal = normal;
}

Vertex::~Vertex()
{
}

Ray::Ray()
{
}

Ray::Ray(const Ray & rhs)
{
	m_origin = rhs.m_origin;
	m_direction = rhs.m_direction;
}

Ray & Ray::operator=(const Ray & rhs)
{
	m_origin = rhs.m_origin;
	m_direction = rhs.m_direction;

	return *this;
}

Ray::~Ray()
{
}

AABB::AABB()
{
	m_center = vec4(0.0, 0.0, 0.0, 1.0);
	m_sphereRadius = 0.0;
	m_boundMin = vec4(0.0, 0.0, 0.0, 1.0);
	m_boundMax = vec4(0.0, 0.0, 0.0, 1.0);
}

AABB::AABB(const AABB & rhs)
{
	m_center = rhs.m_center;
	m_sphereRadius = rhs.m_sphereRadius;
	m_boundMin = rhs.m_boundMin;
	m_boundMax = rhs.m_boundMax;
	m_vertices = rhs.m_vertices;
	m_indices = rhs.m_indices;
}

AABB & AABB::operator=(const AABB & rhs)
{
	m_center = rhs.m_center;
	m_sphereRadius = rhs.m_sphereRadius;
	m_boundMin = rhs.m_boundMin;
	m_boundMax = rhs.m_boundMax;
	m_vertices = rhs.m_vertices;
	m_indices = rhs.m_indices;
	return *this;
}


AABB::~AABB()
{
}

bool AABB::intersectCheck(const AABB & rhs)
{
	if (rhs.m_center.x - m_center.x > rhs.m_sphereRadius + m_sphereRadius) return false;
	if (rhs.m_center.y - m_center.y > rhs.m_sphereRadius + m_sphereRadius) return false;
	if (rhs.m_center.z - m_center.z > rhs.m_sphereRadius + m_sphereRadius) return false;
	else return true;
}

bool AABB::intersectCheck(const Ray & rhs)
{
	double txmin, txmax, tymin, tymax, tzmin, tzmax;
	vec4 l_invDirection = vec4(1 / rhs.m_direction.x, 1 / rhs.m_direction.y, 1 / rhs.m_direction.z, 0.0);
	
	if (l_invDirection.x >= 0.0) {
		txmin = (m_boundMin.x - rhs.m_origin.x) * l_invDirection.x;
		txmax = (m_boundMax.x - rhs.m_origin.x) * l_invDirection.x;
	}
	else {
		txmin = (m_boundMax.x - rhs.m_origin.x) * l_invDirection.x;
		txmax = (m_boundMin.x - rhs.m_origin.x) * l_invDirection.x;
	}
	if (l_invDirection.y >= 0.0) {
		tymin = (m_boundMin.y - rhs.m_origin.y) * l_invDirection.y;
		tymax = (m_boundMax.y - rhs.m_origin.y) * l_invDirection.y;
	}
	else {
		tymin = (m_boundMax.y - rhs.m_origin.y) * l_invDirection.y;
		tymax = (m_boundMin.y - rhs.m_origin.y) * l_invDirection.y;
	}
	if (txmin > tymax || tymin > txmax)
		return false;
	if (l_invDirection.z >= 0.0) {
		tzmin = (m_boundMin.z - rhs.m_origin.z) * l_invDirection.z;
		tzmax = (m_boundMax.z - rhs.m_origin.z) * l_invDirection.z;
	}
	else {
		tzmin = (m_boundMax.z - rhs.m_origin.z) * l_invDirection.z;
		tzmax = (m_boundMin.z - rhs.m_origin.z) * l_invDirection.z;
	}
	if (txmin > tzmax || tzmin > txmax)
		return false;
	txmin = (tymin > txmin) || std::isinf(txmin) ? tymin : txmin;
	txmax = (tymax < txmax) || std::isinf(txmax) ? tymax : txmax;
	txmin = (tzmin > txmin) ? tzmin : txmin;
	txmax = (tzmax < txmax) ? tzmax : txmax;
	if (txmin < txmax && txmax >= 0.0f) {
		return true;
	}
	return false;
}

Transform::Transform()
{
	m_pos = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	m_rot = vec4(0.0f, 0.0f, 0.0f, 1.0f);
	m_scale = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	m_oldPos = m_pos + (1.0f);
	m_oldRot = m_rot.quatMul(0.5f);
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

void Transform::rotate(const vec4 & axis, double angle)
{
	double sinHalfAngle = sin((angle * PI / 180.0) / 2.0);
	double cosHalfAngle = cos((angle * PI / 180.0) / 2.0);
	// get final rotation
	m_rot = vec4(axis.x * sinHalfAngle, axis.y * sinHalfAngle, axis.z * sinHalfAngle, cosHalfAngle).quatMul(m_rot);
}

vec4 & Transform::getPos()
{
	return m_pos;
}

vec4 & Transform::getRot()
{
	return m_rot;
}

vec4 & Transform::getScale()
{
	return m_scale;
}

void Transform::setPos(const vec4 & pos)
{
	m_pos = pos;
}

void Transform::setRot(const vec4 & rot)
{
	m_rot = rot;
}

void Transform::setScale(const vec4 & scale)
{
	m_scale = scale;
}

vec4 & Transform::getOldPos()
{
	return m_oldPos;
}
vec4 & Transform::getOldRot()
{
	return m_oldRot;
}

vec4 & Transform::getOldScale()
{
	return m_oldScale;
}

vec4 Transform::getDirection(direction direction) const
{
	vec4 l_directionVec4;

	switch (direction)
	{
	case FORWARD: l_directionVec4 = vec4(0.0f, 0.0f, 1.0f, 0.0f); break;
	case BACKWARD:l_directionVec4 = vec4(0.0f, 0.0f, -1.0f, 0.0f); break;
	case UP:l_directionVec4 = vec4(0.0f, 1.0f, 0.0f, 0.0f); break;
	case DOWN:l_directionVec4 = vec4(0.0f, -1.0f, 0.0f, 0.0f); break;
	case RIGHT:l_directionVec4 = vec4(1.0f, 0.0f, 0.0f, 0.0f); break;
	case LEFT:l_directionVec4 = vec4(-1.0f, 0.0f, 0.0f, 0.0f); break;
	}

	// V' = QVQ^-1, for unit quaternion, the conjugated quaternion is as same as the inverse quaternion

	// naive version
	// get Q * V by hand
	//vec4 l_hiddenRotatedQuat;
	//l_hiddenRotatedQuat.w = -m_rot.x * l_directionvec4.x - m_rot.y * l_directionvec4.y - m_rot.z * l_directionvec4.z;
	//l_hiddenRotatedQuat.x = m_rot.w * l_directionvec4.x + m_rot.y * l_directionvec4.z - m_rot.z * l_directionvec4.y;
	//l_hiddenRotatedQuat.y = m_rot.w * l_directionvec4.y + m_rot.z * l_directionvec4.x - m_rot.x * l_directionvec4.z;
	//l_hiddenRotatedQuat.z = m_rot.w * l_directionvec4.z + m_rot.x * l_directionvec4.y - m_rot.y * l_directionvec4.x;

	// get conjugated quaternion
	//vec4 l_conjugatedQuat;
	//l_conjugatedQuat = conjugate(m_rot);

	// then QV * Q^-1 
	//vec4 l_directionQuat;
	//l_directionQuat = l_hiddenRotatedQuat * l_conjugatedQuat;
	//l_directionvec4.x = l_directionQuat.x;
	//l_directionvec4.y = l_directionQuat.y;
	//l_directionvec4.z = l_directionQuat.z;

	// traditional version, change direction vector to quaternion representation

	//vec4 l_directionQuat = vec4(0.0, l_directionvec4);
	//l_directionQuat = m_rot * l_directionQuat * conjugate(m_rot);
	//l_directionvec4.x = l_directionQuat.x;
	//l_directionvec4.y = l_directionQuat.y;
	//l_directionvec4.z = l_directionQuat.z;

	// optimized version ([Kavan et al. ] Lemma 4)
	//V' = V + 2 * Qv x (Qv x V + Qs * V)
	vec4 l_Qv = vec4(m_rot.x, m_rot.y, m_rot.z, m_rot.w);
	l_directionVec4 = l_directionVec4 + l_Qv.cross((l_Qv.cross(l_directionVec4) + l_directionVec4.scale(m_rot.w))).scale(2.0f);

	return l_directionVec4;
}


