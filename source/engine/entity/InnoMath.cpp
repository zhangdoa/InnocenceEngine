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

vec4 vec4::operator+(const vec4 & rhs)
{
	return vec4(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
}

vec4 vec4::operator+(double rhs)
{
	return vec4(x + rhs, y + rhs, z + rhs, w + rhs);
}

vec4 vec4::operator-(const vec4 & rhs)
{
	return vec4(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
}

vec4 vec4::operator-(double rhs)
{
	return vec4(x - rhs, y - rhs, z - rhs, w - rhs);
}

double vec4::operator*(const vec4 & rhs)
{
	return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
vec4 vec4::operator*(const mat4 & rhs)
{
	// @TODO: replace with SIMD impl
	vec4 l_vec4;

	l_vec4.x = x * rhs.m[0][0] + y * rhs.m[1][0] + z * rhs.m[2][0] + w * rhs.m[3][0];
	l_vec4.y = x * rhs.m[0][1] + y * rhs.m[1][1] + z * rhs.m[2][1] + w * rhs.m[3][1];
	l_vec4.z = x * rhs.m[0][2] + y * rhs.m[1][2] + z * rhs.m[2][2] + w * rhs.m[3][2];
	l_vec4.w = x * rhs.m[0][3] + y * rhs.m[1][3] + z * rhs.m[2][3] + w * rhs.m[3][3];

	return l_vec4;
}
#endif

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

vec4 vec4::operator/(double rhs)
{
	return vec4(x / rhs, y / rhs, z / rhs, w / rhs);
}

vec4 vec4::quatMul(const vec4 & rhs)
{
	vec4 l_result = vec4(
		w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
		w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
		w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
		w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
	);
	return l_result.normalize();
}

vec4 vec4::quatMul(double rhs)
{
	vec4 l_result = vec4(
		w * rhs + x * rhs + y * rhs - z * rhs,
		w * rhs - x * rhs + y * rhs + z * rhs,
		w * rhs + x * rhs - y * rhs + z * rhs,
		w * rhs - x * rhs - y * rhs - z * rhs
	);
	return l_result.normalize();
}

vec4 vec4::quatConjugate()
{
	return vec4(-x, -y, -z, w);
}

vec4 vec4::reciprocal()
{
	double result_x = 0.0;
	double result_y = 0.0;
	double result_z = 0.0;
	double result_w = 0.0;

	if (x != 0.0)
	{
		result_x = 1.0 / x;
	}
	if (y != 0.0)
	{
		result_y = 1.0 / y;
	}
	if (z != 0.0)
	{
		result_z = 1.0 / z;
	}
	if (w != 0.0)
	{
		result_w = 1.0 / w;
	}

	return vec4(result_x, result_y, result_z, result_w);
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
mat4 vec4::toTranslationMatrix()
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
mat4 vec4::toTranslationMatrix()
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
mat4 vec4::toRotationMatrix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)(1.0 - 2.0 * y * y - 2.0 * z * z);
	l_m.m[0][1] = (float)(2.0 * x * y + 2.0 * z * w);
	l_m.m[0][2] = (float)(2.0 * x * z - 2.0 * y * w);
	l_m.m[0][3] = (float)(0.0);

	l_m.m[1][0] = (float)(2.0 * x * y - 2.0 * z * w);
	l_m.m[1][1] = (float)(1.0 - 2.0 * x * x - 2.0 * z * z);
	l_m.m[1][2] = (float)(2.0 * y * z + 2.0 * x * w);
	l_m.m[1][3] = (float)(0.0);

	l_m.m[2][0] = (float)(2.0 * x * z + 2.0 * y * w);
	l_m.m[2][1] = (float)(2.0 * y * z - 2.0 * x * w);
	l_m.m[2][2] = (float)(1.0 - 2.0 * x * x - 2.0 * y * y);
	l_m.m[2][3] = (float)(0.0);

	l_m.m[3][0] = (float)(0.0);
	l_m.m[3][1] = (float)(0.0);
	l_m.m[3][2] = (float)(0.0);
	l_m.m[3][3] = (float)(1.0);

	return l_m;
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
mat4 vec4::toRotationMatrix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (float)(1.0 - 2.0 * y * y - 2.0 * z * z);
	l_m.m[0][1] = (float)(2.0 * x * y - 2.0 * z * w);
	l_m.m[0][2] = (float)(2.0 * x * z + 2.0 * y * w);
	l_m.m[0][3] = (float)(0.0);

	l_m.m[1][0] = (float)(2.0 * x * y + 2.0 * z * w);
	l_m.m[1][1] = (float)(1.0 - 2.0 * x * x - 2.0 * z * z);
	l_m.m[1][2] = (float)(2.0 * y * z - 2.0 * x * w);
	l_m.m[1][3] = (float)(0.0);

	l_m.m[2][0] = (float)(2.0 * x * z - 2.0 * y * w);
	l_m.m[2][1] = (float)(2.0 * y * z + 2.0 * x * w);
	l_m.m[2][2] = (float)(1.0 - 2.0 * x * x - 2.0 * y * y);
	l_m.m[2][3] = (float)(0);

	l_m.m[3][0] = (float)(0.0);
	l_m.m[3][1] = (float)(0.0);
	l_m.m[3][2] = (float)(0.0);
	l_m.m[3][3] = (float)(1.0);

	return l_m;
}
#endif

mat4 vec4::toScaleMatrix()
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

vec2 vec2::operator+(const vec2 & rhs)
{
	return vec2(x + rhs.x, y + rhs.y);
}

vec2 vec2::operator+(double rhs)
{
	return vec2(x + rhs, y + rhs);
}

vec2 vec2::operator-(const vec2 & rhs)
{
	return vec2(x - rhs.x, y - rhs.y);
}

vec2 vec2::operator-(double rhs)
{
	return vec2(x - rhs, y - rhs);
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

vec2 vec2::operator/(double rhs)
{
	return vec2(x / rhs, y / rhs);
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
	l_m.m[0][1] = m[0][0] * rhs.m[0][1] + m[0][1] * rhs.m[1][1] + m[0][2] * rhs.m[2][1] + m[0][3] * rhs.m[3][1];
	l_m.m[0][2] = m[0][0] * rhs.m[0][2] + m[0][1] * rhs.m[1][2] + m[0][2] * rhs.m[2][2] + m[0][3] * rhs.m[3][2];
	l_m.m[0][3] = m[0][0] * rhs.m[0][3] + m[0][1] * rhs.m[1][3] + m[0][2] * rhs.m[2][3] + m[0][3] * rhs.m[3][3];

	l_m.m[1][0] = m[1][0] * rhs.m[0][0] + m[1][1] * rhs.m[1][0] + m[1][2] * rhs.m[2][0] + m[1][3] * rhs.m[3][0];
	l_m.m[1][1] = m[1][0] * rhs.m[0][1] + m[1][1] * rhs.m[1][1] + m[1][2] * rhs.m[2][1] + m[1][3] * rhs.m[3][1];
	l_m.m[1][2] = m[1][0] * rhs.m[0][2] + m[1][1] * rhs.m[1][2] + m[1][2] * rhs.m[2][2] + m[1][3] * rhs.m[3][2];
	l_m.m[1][3] = m[1][0] * rhs.m[0][3] + m[1][1] * rhs.m[1][3] + m[1][2] * rhs.m[2][3] + m[1][3] * rhs.m[3][3];

	l_m.m[2][0] = m[2][0] * rhs.m[0][0] + m[2][1] * rhs.m[1][0] + m[2][2] * rhs.m[2][0] + m[2][3] * rhs.m[3][0];
	l_m.m[2][1] = m[2][0] * rhs.m[0][1] + m[2][1] * rhs.m[1][1] + m[2][2] * rhs.m[2][1] + m[2][3] * rhs.m[3][1];
	l_m.m[2][2] = m[2][0] * rhs.m[0][2] + m[2][1] * rhs.m[1][2] + m[2][2] * rhs.m[2][2] + m[2][3] * rhs.m[3][2];
	l_m.m[2][3] = m[2][0] * rhs.m[0][3] + m[2][1] * rhs.m[1][3] + m[2][2] * rhs.m[2][3] + m[2][3] * rhs.m[3][3];

	l_m.m[3][0] = m[3][0] * rhs.m[0][0] + m[3][1] * rhs.m[1][0] + m[3][2] * rhs.m[2][0] + m[3][3] * rhs.m[3][0];
	l_m.m[3][1] = m[3][0] * rhs.m[0][1] + m[3][1] * rhs.m[1][1] + m[3][2] * rhs.m[2][1] + m[3][3] * rhs.m[3][1];
	l_m.m[3][2] = m[3][0] * rhs.m[0][2] + m[3][1] * rhs.m[1][2] + m[3][2] * rhs.m[2][2] + m[3][3] * rhs.m[3][2];
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

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
vec4 mat4::operator*(const vec4 & rhs)
{
	// @TODO: replace with SIMD impl
	vec4 l_vec4;

	l_vec4.x = m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3] * rhs.w;
	l_vec4.y = m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3] * rhs.w;
	l_vec4.z = m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3] * rhs.w;
	l_vec4.w = m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3] * rhs.w;

	return l_vec4;
}
#endif

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

	l_m.m[0][0] = m[2][1] * m[3][2] * m[1][3] - m[3][1] * m[2][2] * m[1][3] + m[3][1] * m[1][2] * m[2][3] - m[1][1] * m[3][2] * m[2][3] - m[2][1] * m[1][2] * m[3][3] + m[1][1] * m[2][2] * m[3][3];
	l_m.m[1][0] = m[3][0] * m[2][2] * m[1][3] - m[2][0] * m[3][2] * m[1][3] - m[3][0] * m[1][2] * m[2][3] + m[1][0] * m[3][2] * m[2][3] + m[2][0] * m[1][2] * m[3][3] - m[1][0] * m[2][2] * m[3][3];
	l_m.m[2][0] = m[2][0] * m[3][1] * m[1][3] - m[3][0] * m[2][1] * m[1][3] + m[3][0] * m[1][1] * m[2][3] - m[1][0] * m[3][1] * m[2][3] - m[2][0] * m[1][1] * m[3][3] + m[1][0] * m[2][1] * m[3][3];
	l_m.m[3][0] = m[3][0] * m[2][1] * m[1][2] - m[2][0] * m[3][1] * m[1][2] - m[3][0] * m[1][1] * m[2][2] + m[1][0] * m[3][1] * m[2][2] + m[2][0] * m[1][1] * m[3][2] - m[1][0] * m[2][1] * m[3][2];
	l_m.m[0][1] = m[3][1] * m[2][2] * m[0][3] - m[2][1] * m[3][2] * m[0][3] - m[3][1] * m[0][2] * m[2][3] + m[0][1] * m[3][2] * m[2][3] + m[2][1] * m[0][2] * m[3][3] - m[0][1] * m[2][2] * m[3][3];
	l_m.m[1][1] = m[2][0] * m[3][2] * m[0][3] - m[3][0] * m[2][2] * m[0][3] + m[3][0] * m[0][2] * m[2][3] - m[0][0] * m[3][2] * m[2][3] - m[2][0] * m[0][2] * m[3][3] + m[0][0] * m[2][2] * m[3][3];
	l_m.m[2][1] = m[3][0] * m[2][1] * m[0][3] - m[2][0] * m[3][1] * m[0][3] - m[3][0] * m[0][1] * m[2][3] + m[0][0] * m[3][1] * m[2][3] + m[2][0] * m[0][1] * m[3][3] - m[0][0] * m[2][1] * m[3][3];
	l_m.m[3][1] = m[2][0] * m[3][1] * m[0][2] - m[3][0] * m[2][1] * m[0][2] + m[3][0] * m[0][1] * m[2][2] - m[0][0] * m[3][1] * m[2][2] - m[2][0] * m[0][1] * m[3][2] + m[0][0] * m[2][1] * m[3][2];
	l_m.m[0][2] = m[1][1] * m[3][2] * m[0][3] - m[3][1] * m[1][2] * m[0][3] + m[3][1] * m[0][2] * m[1][3] - m[0][1] * m[3][2] * m[1][3] - m[1][1] * m[0][2] * m[3][3] + m[0][1] * m[1][2] * m[3][3];
	l_m.m[1][2] = m[3][0] * m[1][2] * m[0][3] - m[1][0] * m[3][2] * m[0][3] - m[3][0] * m[0][2] * m[1][3] + m[0][0] * m[3][2] * m[1][3] + m[1][0] * m[0][2] * m[3][3] - m[0][0] * m[1][2] * m[3][3];
	l_m.m[2][2] = m[1][0] * m[3][1] * m[0][3] - m[3][0] * m[1][1] * m[0][3] + m[3][0] * m[0][1] * m[1][3] - m[0][0] * m[3][1] * m[1][3] - m[1][0] * m[0][1] * m[3][3] + m[0][0] * m[1][1] * m[3][3];
	l_m.m[3][2] = m[3][0] * m[1][1] * m[0][2] - m[1][0] * m[3][1] * m[0][2] - m[3][0] * m[0][1] * m[1][2] + m[0][0] * m[3][1] * m[1][2] + m[1][0] * m[0][1] * m[3][2] - m[0][0] * m[1][1] * m[3][2];
	l_m.m[0][3] = m[2][1] * m[1][2] * m[0][3] - m[1][1] * m[2][2] * m[0][3] - m[2][1] * m[0][2] * m[1][3] + m[0][1] * m[2][2] * m[1][3] + m[1][1] * m[0][2] * m[2][3] - m[0][1] * m[1][2] * m[2][3];
	l_m.m[1][3] = m[1][0] * m[2][2] * m[0][3] - m[2][0] * m[1][2] * m[0][3] + m[2][0] * m[0][2] * m[1][3] - m[0][0] * m[2][2] * m[1][3] - m[1][0] * m[0][2] * m[2][3] + m[0][0] * m[1][2] * m[2][3];
	l_m.m[2][3] = m[2][0] * m[1][1] * m[0][3] - m[1][0] * m[2][1] * m[0][3] - m[2][0] * m[0][1] * m[1][3] + m[0][0] * m[2][1] * m[1][3] + m[1][0] * m[0][1] * m[2][3] - m[0][0] * m[1][1] * m[2][3];
	l_m.m[3][3] = m[1][0] * m[2][1] * m[0][2] - m[2][0] * m[1][1] * m[0][2] + m[2][0] * m[0][1] * m[1][2] - m[0][0] * m[2][1] * m[1][2] - m[1][0] * m[0][1] * m[2][2] + m[0][0] * m[1][1] * m[2][2];

	l_m = (l_m * (1 / this->getDeterminant()));
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

void mat4::initializeToIdentityMatrix()
{
	m[0][0] = 1.0;
	m[1][1] = 1.0;
	m[2][2] = 1.0;
	m[3][3] = 1.0;
}

//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double WHRatio, double zNear, double zFar)
{
	m[0][0] = (float)(1.0 / (tan(FOV / 2.0) * WHRatio));
	m[1][1] = (float)(1.0 / tan(FOV / 2.0));
	m[2][2] = (float)(-(zFar + zNear) / ((zFar - zNear)));
	m[2][3] = (float)-1.0;
	m[3][2] = (float)(-(2.0 * zFar * zNear) / ((zFar - zNear)));
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double WHRatio, double zNear, double zFar)
{
	m[0][0] = (float)(1.0 / (tan(FOV / 2.0) * WHRatio));
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
	m[3][0] = (float)(-(right + left) / (right - left));
	m[3][1] = (float)(-(up + bottom) / (up - bottom));
	m[3][2] = (float)(-(zFar + zNear) / (zFar - zNear));
	m[3][3] = (float)1.0;
}
#endif

//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar)
{
	m[0][0] = (float)(2.0 / (right - left));
	m[0][3] = (float)(-(right + left) / (right - left));
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
	vec4 l_Y = upDir;
	vec4 l_Z = vec4(centerPos.x - eyePos.x, centerPos.y - eyePos.y, centerPos.z - eyePos.z, 0.0).normalize();

	l_X = l_Z.cross(l_Y);
	l_X = l_X.normalize();
	l_Y = l_X.cross(l_Z);
	l_Y = l_Y.normalize();

	l_m.m[0][0] = (float)l_X.x;
	l_m.m[0][1] = (float)l_Y.x;
	l_m.m[0][2] = (float)-l_Z.x;
	l_m.m[0][3] = 0.0f;
	l_m.m[1][0] = (float)l_X.y;
	l_m.m[1][1] = (float)l_Y.y;
	l_m.m[1][2] = (float)-l_Z.y;
	l_m.m[1][3] = 0.0f;
	l_m.m[2][0] = (float)l_X.z;
	l_m.m[2][1] = (float)l_Y.z;
	l_m.m[2][2] = (float)-l_Z.z;
	l_m.m[2][3] = 0.0f;
	l_m.m[3][0] = (float)-(l_X * vec4(eyePos.x, eyePos.y, eyePos.z, 0.0));
	l_m.m[3][1] = (float)-(l_Y * vec4(eyePos.x, eyePos.y, eyePos.z, 0.0));
	l_m.m[3][2] = (float)(l_Z * vec4(eyePos.x, eyePos.y, eyePos.z, 0.0));
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
	vec4 l_Y = upDir;
	vec4 l_Z = vec4(centerPos.x - eyePos.x, centerPos.y - eyePos.y, centerPos.z - eyePos.z, 0.0).normalize();

	l_X = l_Z.cross(l_Y);
	l_X = l_X.normalize();
	l_Y = l_X.cross(l_Z);
	l_Y = l_Y.normalize();

	l_m.m[0][0] = (float)l_X.x;
	l_m.m[0][1] = (float)l_X.y;
	l_m.m[0][2] = (float)l_X.z;
	l_m.m[0][3] = (float)-(l_X * vec4(eyePos.x, eyePos.y, eyePos.z, 0.0));
	l_m.m[1][0] = (float)l_Y.x;
	l_m.m[1][1] = (float)l_Y.y;
	l_m.m[1][2] = (float)l_Y.z;
	l_m.m[1][3] = (float)-(l_Y * vec4(eyePos.x, eyePos.y, eyePos.z, 0.0));
	l_m.m[2][0] = (float)-l_Z.x;
	l_m.m[2][1] = (float)-l_Z.y;
	l_m.m[2][2] = (float)-l_Z.z;
	l_m.m[2][3] = (float)(l_Z * vec4(eyePos.x, eyePos.y, eyePos.z, 0.0));
	l_m.m[3][0] = 0.0f;
	l_m.m[3][1] = 0.0f;
	l_m.m[3][2] = 0.0f;
	l_m.m[3][3] = 1.0f;

	return l_m;
}
#endif

Vertex& Vertex::operator=(const Vertex & rhs)
{
	m_pos = rhs.m_pos;
	m_texCoord = rhs.m_texCoord;
	m_normal = rhs.m_normal;
	return *this;
}

Ray & Ray::operator=(const Ray & rhs)
{
	m_origin = rhs.m_origin;
	m_direction = rhs.m_direction;

	return *this;
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
	double l_invDirectionX = std::isinf(1.0 / rhs.m_direction.x) ? 0.0 : 1.0 / rhs.m_direction.x;
	double l_invDirectionY = std::isinf(1.0 / rhs.m_direction.y) ? 0.0 : 1.0 / rhs.m_direction.y;
	double l_invDirectionZ = std::isinf(1.0 / rhs.m_direction.z) ? 0.0 : 1.0 / rhs.m_direction.z;
	vec4 l_invDirection = vec4(l_invDirectionX, l_invDirectionY, l_invDirectionZ, 0.0).normalize();

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
	if (txmin < txmax && txmax >= 0.0) {
		return true;
	}
	return false;
}

bool Transform::hasChanged()
{
	if (m_pos != m_previousPos || m_rot != m_previousRot || m_scale != m_previousScale)
	{
		return true;
	}

	if (nullptr != m_parentTransform)
	{
		if (m_parentTransform->hasChanged())
		{
			return true;
		}
	}
	return false;

}

void Transform::update()
{
	m_previousPos = m_pos;
	m_previousRot = m_rot;
	m_previousScale = m_scale;
}

void Transform::rotateInLocal(const vec4 & axis, double angle)
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

void Transform::setLocalPos(const vec4 & pos)
{
	m_pos = pos;
}

void Transform::setLocalRot(const vec4 & rot)
{
	m_rot = rot;
}

void Transform::setLocalScale(const vec4 & scale)
{
	m_scale = scale;
}

mat4 Transform::caclLocalTranslationMatrix()
{
	return m_pos.toTranslationMatrix();
}

mat4 Transform::caclLocalRotMatrix()
{
	return m_rot.toRotationMatrix();
}

mat4 Transform::caclLocalScaleMatrix()
{
	return m_scale.toScaleMatrix();
}

mat4 Transform::caclLocalTransformationMatrix()
{
	return caclLocalTranslationMatrix() * caclLocalRotMatrix() * caclLocalScaleMatrix();
}

mat4 Transform::caclPreviousLocalTranslationMatrix()
{
	return m_previousPos.toTranslationMatrix();
}

mat4 Transform::caclPreviousLocalRotMatrix()
{
	return m_previousRot.toRotationMatrix();
}

mat4 Transform::caclPreviousLocalScaleMatrix()
{
	return m_previousScale.toScaleMatrix();
}

mat4 Transform::caclPreviousLocalTransformationMatrix()
{
	return caclPreviousLocalTranslationMatrix() * caclPreviousLocalRotMatrix() * caclPreviousLocalScaleMatrix();
}

vec4 Transform::caclGlobalPos()
{
	mat4 l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclGlobalTransformationMatrix();
	}

	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	auto result = vec4();
	result = m_pos * l_parentTransformationMatrix;
	result = result * (1 / result.w);
	return result;
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	auto result = vec4();
	result = l_parentTransformationMatrix * m_pos;
	result = result * (1 / result.w);
	return result;
#endif
}

vec4 Transform::caclGlobalRot()
{
	vec4 l_parentRot = vec4(0.0, 0.0, 0.0, 1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentRot = m_parentTransform->caclGlobalRot();
	}

	return l_parentRot.quatMul(m_rot);
}

vec4 Transform::caclGlobalScale()
{
	vec4 l_parentScale = vec4(1.0, 1.0, 1.0, 1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentScale = m_parentTransform->caclGlobalScale();
	}

	return l_parentScale.scale(m_scale);
}

vec4 Transform::caclPreviousGlobalPos()
{
	mat4 l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclPreviousGlobalTransformationMatrix();
	}

	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	auto result = vec4();
	result = m_previousPos * l_parentTransformationMatrix;
	result = result * (1 / result.w);
	return result;
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	auto result = vec4();
	result = l_parentTransformationMatrix * m_previousPos;
	result = result * (1 / result.w);
	return result;
#endif
}

vec4 Transform::caclPreviousGlobalRot()
{
	vec4 l_parentRot = vec4(0.0, 0.0, 0.0, 1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentRot = m_parentTransform->caclPreviousGlobalRot();
	}

	return l_parentRot.quatMul(m_previousRot);
}

vec4 Transform::caclPreviousGlobalScale()
{
	vec4 l_parentScale = vec4(1.0, 1.0, 1.0, 1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentScale = m_parentTransform->caclPreviousGlobalScale();
	}

	return l_parentScale.scale(m_previousScale);
}

mat4 Transform::caclGlobalTranslationMatrix()
{
	return caclGlobalPos().toTranslationMatrix();
}

mat4 Transform::caclGlobalRotMatrix()
{
	return caclGlobalRot().toRotationMatrix();
}

mat4 Transform::caclGlobalScaleMatrix()
{
	return caclGlobalScale().toScaleMatrix();
}

mat4 Transform::caclGlobalTransformationMatrix()
{
	mat4 l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclGlobalTransformationMatrix();
	}

	return l_parentTransformationMatrix * caclLocalTransformationMatrix();
}

mat4 Transform::caclPreviousGlobalTranslationMatrix()
{
	return caclPreviousGlobalPos().toTranslationMatrix();
}

mat4 Transform::caclPreviousGlobalRotMatrix()
{
	return caclPreviousGlobalRot().toRotationMatrix();
}

mat4 Transform::caclPreviousGlobalScaleMatrix()
{
	return caclPreviousGlobalScale().toScaleMatrix();
}

mat4 Transform::caclPreviousGlobalTransformationMatrix()
{
	mat4 l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclPreviousGlobalTransformationMatrix();
	}

	return l_parentTransformationMatrix * caclPreviousLocalTransformationMatrix();

}

mat4 Transform::caclLookAtMatrix()
{
	return mat4().lookAt(caclGlobalPos(), caclGlobalPos() + getDirection(direction::BACKWARD), getDirection(direction::UP));
}

mat4 Transform::caclPreviousLookAtMatrix()
{
	return mat4().lookAt(caclPreviousGlobalPos(), caclPreviousGlobalPos() + getPreviousDirection(direction::BACKWARD), getPreviousDirection(direction::UP));
}

mat4 Transform::getInvertLocalTranslationMatrix()
{
	return m_pos.scale(-1.0).toTranslationMatrix();
}

mat4 Transform::getInvertLocalRotMatrix()
{
	return m_rot.quatConjugate().toRotationMatrix();
}

mat4 Transform::getInvertLocalScaleMatrix()
{
	return m_scale.reciprocal().toScaleMatrix();
}

mat4 Transform::getInvertGlobalTranslationMatrix()
{
	return caclGlobalPos().scale(-1.0).toTranslationMatrix();
}

mat4 Transform::getInvertGlobalRotMatrix()
{
	return caclGlobalRot().quatConjugate().toRotationMatrix();
}

mat4 Transform::getInvertGlobalScaleMatrix()
{
	return caclGlobalScale().reciprocal().toScaleMatrix();
}

mat4 Transform::getPreviousInvertLocalTranslationMatrix()
{
	return m_previousPos.scale(-1.0).toTranslationMatrix();
}

mat4 Transform::getPreviousInvertLocalRotMatrix()
{
	return m_previousRot.quatConjugate().toRotationMatrix();
}

mat4 Transform::getPreviousInvertLocalScaleMatrix()
{
	return m_previousScale.reciprocal().toScaleMatrix();
}

mat4 Transform::getPreviousInvertGlobalTranslationMatrix()
{
	return caclPreviousGlobalPos().scale(-1.0).toTranslationMatrix();
}

mat4 Transform::getPreviousInvertGlobalRotMatrix()
{
	return caclPreviousGlobalRot().quatConjugate().toRotationMatrix();
}

mat4 Transform::getPreviousInvertGlobalScaleMatrix()
{
	return caclPreviousGlobalScale().reciprocal().toScaleMatrix();
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

	return l_directionVec4.normalize();
}

vec4 Transform::getPreviousDirection(direction direction) const
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
	vec4 l_Qv = vec4(m_previousRot.x, m_previousRot.y, m_previousRot.z, m_previousRot.w);
	l_directionVec4 = l_directionVec4 + l_Qv.cross((l_Qv.cross(l_directionVec4) + l_directionVec4.scale(m_previousRot.w))).scale(2.0f);

	return l_directionVec4.normalize();

}


