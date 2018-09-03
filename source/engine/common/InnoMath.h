#pragma once
#include "../common/stdafx.h"
#include "../common/config.h"

//typedef __m128 TVec4;

template<class T>
const static T PI = T(3.14159265358979323846264338327950288L);

template<class T>
T tolerance2 = T(0.95L);

template<class T>
T tolerance4 = T(0.9995L);

template<class T>
T tolerance8 = T(0.99999995L);

template<class T>
T zero = T(0.0L);

template<class T>
T one = T(1.0L);

template<class T>
T two = T(2.0L);

template<class T>
T halfCircumference = T(180.0L);

template<class T>
class TVec2
{
public:

	T x;
	T y;

	TVec2()
	{
		x = zero<T>();
		y = zero<T>();
	}

	TVec2(T rhsX, T rhsY)
	{
		x = rhsX;
		y = rhsY;
	}

	TVec2(const TVec2<T>& rhs)
	{
		x = rhs.x;
		y = rhs.y;
	}

	auto operator=(const TVec2<T>& rhs) -> TVec2<T>&
	{
		x = rhs.x;
		y = rhs.y;
		return *this;
	}

	~TVec2()
	{
	}

	auto operator+(const TVec2<T>& rhs) -> TVec2<T>
	{
		return TVec2<T>(x + rhs.x, y + rhs.y);
	}

	auto operator+(T rhs) -> TVec2<T>
	{
		return TVec2<T>(x + rhs, y + rhs);
	}

	auto operator-(const TVec2<T>& rhs) -> TVec2<T>
	{
		return TVec2<T>(x - rhs.x, y - rhs.y);
	}

	auto operator-(T rhs) -> TVec2<T>
	{
		return TVec2<T>(x - rhs, y - rhs);
	}

	auto operator*(const TVec2<T>& rhs) -> T
	{
		return x * rhs.x + y * rhs.y;
	}

	auto scale(const TVec2<T>& rhs) -> TVec2<T>
	{
		return TVec2<T>(x * rhs.x, y * rhs.y);
	}

	auto scale(T rhs) -> TVec2<T>
	{
		return TVec2<T>(x * rhs, y * rhs);
	}
	auto operator*(T rhs) -> TVec2<T>
	{
		return TVec2<T>(x * rhs, y * rhs);
	}

	auto operator/(T rhs) -> TVec2<T>
	{
		return TVec2<T>(x / rhs, y / rhs);
	}

	auto length() -> T
	{
		return std::sqrt(x * x + y * y);
	}
	auto normalize() -> TVec2<T>
	{
		return TVec2<T>(x / length(), y / length());
	}
};

template <class T>
// In Homogeneous Coordinates, the w component is a scalar of x, y and z, to represent 3D point vector in 4D, set w to 1.0; to represent 3D direction vector in 4D, set w to 0.0.
// In Quaternion, the w component is sin(theta / 2).
class TVec4
{
public:

	T x;
	T y;
	T z;
	T w;

	TVec4()
	{
		x = T();
		y = T();
		z = T();
		w = T();
	}

	TVec4(T rhsX, T rhsY, T rhsZ, T rhsW)
	{
		x = rhsX;
		y = rhsY;
		z = rhsZ;
		w = rhsW;
	}
	TVec4(const TVec4& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
	}
	auto operator=(const TVec4<T> & rhs) -> TVec4<T>&
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
		return *this;
	}

	~TVec4()
	{
	}

	auto operator+(const TVec4<T> & rhs) const -> TVec4<T>
	{
		return TVec4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}

	auto operator+(T rhs) const -> TVec4<T>
	{
		return TVec4<T>(x + rhs, y + rhs, z + rhs, w + rhs);
	}

	auto operator-(const TVec4<T> & rhs) const -> TVec4<T>
	{
		return TVec4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
	}

	auto operator-(T rhs) const -> TVec4<T>
	{
		return TVec4<T>(x - rhs, y - rhs, z - rhs, w - rhs);
	}

	auto operator*(const TVec4<T> & rhs)const -> T
	{
		return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
	}

	auto cross(const TVec4<T>& rhs) const -> TVec4<T>
	{
		return TVec4<T>(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x, T());
	}

	auto scale(const TVec4<T>& rhs) const -> TVec4<T>
	{
		return TVec4<T>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
	}

	auto scale(T rhs) const -> TVec4<T>
	{
		return TVec4<T>(x * rhs, y * rhs, z * rhs, w * rhs);
	}

	auto operator*(T rhs) const -> TVec4<T>
	{
		return TVec4<T>(x * rhs, y * rhs, z * rhs, w * rhs);
	}

	auto operator/(T rhs) const -> TVec4<T>
	{
		return TVec4<T>(x / rhs, y / rhs, z / rhs, w / rhs);
	}

	auto quatMul(const TVec4<T>& rhs) const -> TVec4<T>
	{
		TVec4<T> l_result = TVec4<T>(
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
			w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
			w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
			);
		return l_result.normalize();
	}

	auto quatMul(T rhs) const -> TVec4<T>
	{
		TVec4<T> l_result = TVec4<T>(
			w * rhs + x * rhs + y * rhs - z * rhs,
			w * rhs - x * rhs + y * rhs + z * rhs,
			w * rhs + x * rhs - y * rhs + z * rhs,
			w * rhs - x * rhs - y * rhs - z * rhs
			);
		return l_result.normalize();
	}

	auto quatConjugate() const -> TVec4<T>
	{
		return TVec4<T>(-x, -y, -z, w);
	}

	auto reciprocal() const -> TVec4<T>
	{
		T result_x = T();
		T result_y = T();
		T result_z = T();
		T result_w = T();

		if (x != T())
		{
			result_x = one<T> / x;
		}
		if (y != T())
		{
			result_y = one<T> / y;
		}
		if (z != T())
		{
			result_z = one<T> / z;
		}
		if (w != T())
		{
			result_w = one<T> / w;
		}

		return TVec4<T>(result_x, result_y, result_z, result_w);
	}

	auto length() -> T
	{
		// @TODO: replace with SIMD impl
		return std::sqrt(x * x + y * y + z * z + w * w);
	}

	auto normalize() -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		auto l_length = length();
		return TVec4(x / l_length, y / l_length, z / l_length, w / l_length);
	}

	auto lerp(const TVec4<T>& a, const TVec4<T>& b, T alpha) -> TVec4<T>
	{
		return a * alpha + b * (one<T> -alpha);
	}

	auto slerp(const TVec4<T>& a, const TVec4<T>& b, T alpha) -> TVec4<T>
	{
		T cosOfAngle = a * b;
		// use nlerp for quaternions which are too close 
		if (cosOfAngle > tolerance4<T>) {
			return (a * alpha + b * (one<T>-alpha)).normalize();
		}
		// for shorter path
		if (cosOfAngle < T()) {
			auto theta_0 = acos(-cosOfAngle);
			auto theta = theta_0 * alpha;
			auto sin_theta = sin(theta);
			auto sin_theta_0 = sin(theta_0);

			auto s0 = sin_theta / sin_theta_0;
			auto s1 = cos(theta) + cosOfAngle * sin_theta / sin_theta_0;

			return (a * -one<T> * s0) + (b * s1);
		}
		else
		{
			auto theta_0 = acos(cosOfAngle);
			auto theta = theta_0 * alpha;
			auto sin_theta = std::sin(theta);
			auto sin_theta_0 = std::sin(theta_0);

			auto s0 = sin_theta / sin_theta_0;
			auto s1 = std::cos(theta) - cosOfAngle * sin_theta / sin_theta_0;

			return (a * s0) + (b * s1);
		}
	}

	auto nlerp(const TVec4<T>& a, const TVec4<T>& b, T alpha) -> TVec4<T>
	{
		return (a * alpha + b * (one<T> -alpha)).normalize();
	}

	bool operator!=(const TVec4<T> & rhs)
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

	bool operator==(const TVec4<T> & rhs)
	{
		// @TODO: replace with SIMD impl
		return !(*this != rhs);
	}
};

/*

matrix4x4 mathematical convention :
| a00 a01 a02 a03 |
| a10 a11 a12 a13 |
| a20 a21 a22 a23 |
| a30 a31 a32 a33 |

*/

/* Column-Major vector4 mathematical convention

vector4(a matrix4x1) :
| x |
| y |
| z |
| w |

use right/post-multiplication, need to access each rows of the matrix then each elements,
for C/C++, it's cache-friendly with Row-Major memory layout;
for Fortan/Matlab, it's cache-friendly with Column-Major memory layout.

matrix4x4 * vector4 :
| x' = a00 * x  + a01 * y + a02 * z + a03 * w |
| y' = a10 * x  + a11 * y + a12 * z + a13 * w |
| z' = a20 * x  + a21 * y + a22 * z + a23 * w |
| w' = a30 * x  + a31 * y + a32 * z + a33 * w |

*/

/* Row-Major vector4 mathematical convention

vector4(a matrix1x4) :
| x y z w |

use left/pre-multiplication, need to access each columns of the matrix then each elements,
for C/C++, it's cache-friendly with Column-Major memory layout;
for Fortan/Matlab, it's cache-friendly with Row-Major memory layout.

vector4 * matrix4x4 :
| x' = x * a00 + y * a10 + z * a20 + w * a30 |
| y' = x * a01 + y * a11 + z * a21 + w * a31 |
| z' = x * a02 + y * a12 + z * a22 + w * a32 |
| w' = x * a03 + y * a13 + z * a23 + w * a33 |

*/

/* Column-Major memory layout (in C/C++)
matrix4x4 :
[columnIndex][rowIndex]
| m[0][0] <-> a00 m[1][0] <-> a01 m[2][0] <-> a02 m[3][0] <-> a03 |
| m[0][1] <-> a10 m[1][1] <-> a11 m[2][1] <-> a12 m[3][1] <-> a13 |
| m[0][2] <-> a20 m[1][2] <-> a21 m[2][2] <-> a22 m[3][2] <-> a23 |
| m[0][3] <-> a30 m[1][3] <-> a31 m[2][3] <-> a32 m[3][3] <-> a33 |
vector4 :
m[0][0] <-> x m[1][0] <-> y m[2][0] <-> z m[3][0] <-> w
*/

/* Row-Major memory layout (in C/C++)
matrix4x4 :
[rowIndex][columnIndex]
| m[0][0] <-> a00 m[0][1] <-> a01 m[0][2] <-> a02 m[0][3] <-> a03 |
| m[1][0] <-> a10 m[1][1] <-> a11 m[1][2] <-> a12 m[1][3] <-> a13 |
| m[2][0] <-> a20 m[2][1] <-> a21 m[2][2] <-> a22 m[2][3] <-> a23 |
| m[3][0] <-> a30 m[3][1] <-> a31 m[3][2] <-> a32 m[3][3] <-> a33 |
vector4 :
m[0][0] <-> x m[0][1] <-> y m[0][2] <-> z m[0][3] <-> w  (best choice)
*/

template<class T>
class TMat4
{
public:
	T m[4][4];

	TMat4()
	{
		m[0][0] = T();
		m[0][1] = T();
		m[0][2] = T();
		m[0][3] = T();
		m[1][0] = T();
		m[1][1] = T();
		m[1][2] = T();
		m[1][3] = T();
		m[2][0] = T();
		m[2][1] = T();
		m[2][2] = T();
		m[2][3] = T();
		m[3][0] = T();
		m[3][1] = T();
		m[3][2] = T();
		m[3][3] = T();
	}
	TMat4(const TMat4<T>& rhs)
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
	auto operator=(const TMat4<T>& rhs) -> TMat4<T>&
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
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	auto TMat4::operator*(const TMat4<T> & rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

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
	//Row-Major memory layout
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	auto TMat4::operator*(const TMat4<T> & rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

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

	auto operator*(T rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

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

	auto transpose() -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

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
	auto inverse() -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

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

		l_m = (l_m * (one<T> / this->getDeterminant()));
		return l_m;
	}
	auto getDeterminant() -> T
	{
		T value;

		value =
			m[3][0] * m[2][1] * m[1][2] * m[0][3] - m[2][0] * m[3][1] * m[1][2] * m[0][3] - m[3][0] * m[1][1] * m[2][2] * m[0][3] + m[1][0] * m[3][1] * m[2][2] * m[0][3] +
			m[2][0] * m[1][1] * m[3][2] * m[0][3] - m[1][0] * m[2][1] * m[3][2] * m[0][3] - m[3][0] * m[2][1] * m[0][2] * m[1][3] + m[2][0] * m[3][1] * m[0][2] * m[1][3] +
			m[3][0] * m[0][1] * m[2][2] * m[1][3] - m[0][0] * m[3][1] * m[2][2] * m[1][3] - m[2][0] * m[0][1] * m[3][2] * m[1][3] + m[0][0] * m[2][1] * m[3][2] * m[1][3] +
			m[3][0] * m[1][1] * m[0][2] * m[2][3] - m[1][0] * m[3][1] * m[0][2] * m[2][3] - m[3][0] * m[0][1] * m[1][2] * m[2][3] + m[0][0] * m[3][1] * m[1][2] * m[2][3] +
			m[1][0] * m[0][1] * m[3][2] * m[2][3] - m[0][0] * m[1][1] * m[3][2] * m[2][3] - m[2][0] * m[1][1] * m[0][2] * m[3][3] + m[1][0] * m[2][1] * m[0][2] * m[3][3] +
			m[2][0] * m[0][1] * m[1][2] * m[3][3] - m[0][0] * m[2][1] * m[1][2] * m[3][3] - m[1][0] * m[0][1] * m[2][2] * m[3][3] + m[0][0] * m[1][1] * m[2][2] * m[3][3];

		return value;
	}
	/*
	Column-Major memory layout and
	Row-Major vector4 mathematical convention

	vector4(a matrix1x4) :
	| x y z w |

	matrix4x4 £º
	[columnIndex][rowIndex]
	| m[0][0] <-> a00(1.0 / (tan(FOV / 2.0) * HWRatio)) m[1][0] <->  a01(         0.0         ) m[2][0] <->  a02(                   0.0                  ) m[3][0] <->  a03(                   0.0                  ) |
	| m[0][1] <-> a10(               0.0              ) m[1][1] <->  a11(1.0 / (tan(FOV / 2.0)) m[2][1] <->  a12(                   0.0                  ) m[3][1] <->  a13(                   0.0                  ) |
	| m[0][2] <-> a20(               0.0              ) m[1][2] <->  a21(         0.0         ) m[2][2] <->  a22(   -(zFar + zNear) / ((zFar - zNear))   ) m[3][2] <->  a23(-(2.0 * zFar * zNear) / ((zFar - zNear))) |
	| m[0][3] <-> a30(               0.0              ) m[1][3] <->  a31(         0.0         ) m[2][3] <->  a32(                  -1.0                  ) m[3][3] <->  a33(                   1.0                  ) |

	in

	vector4 * matrix4x4 :

	--------------------------------------------

	Row-Major memory layout and
	Column-Major vector4 mathematical convention

	vector4(a matrix4x1) :
	| x |
	| y |
	| z |
	| w |

	matrix4x4 £º
	[rowIndex][columnIndex]
	| m[0][0] <-> a00(1.0 / (tan(FOV / 2.0) * HWRatio)) m[1][0] <->  a01(         0.0         ) m[2][0] <->  a02(                   0.0                  ) m[3][0] <->  a03( 0.0) |
	| m[0][1] <-> a01(               0.0              ) m[1][1] <->  a11(1.0 / (tan(FOV / 2.0)) m[2][1] <->  a21(                   0.0                  ) m[3][1] <->  a31( 0.0) |
	| m[0][2] <-> a02(               0.0              ) m[1][2] <->  a12(         0.0         ) m[2][2] <->  a22(   -(zFar + zNear) / ((zFar - zNear))   ) m[3][2] <->  a32(-1.0) |
	| m[0][3] <-> a03(               0.0              ) m[1][3] <->  a13(         0.0         ) m[2][3] <->  a23(-(2.0 * zFar * zNear) / ((zFar - zNear))) m[3][3] <->  a33( 1.0) |

	in

	matrix4x4 * vector4 :
	*/


	void initializeToIdentityMatrix()
	{
		m[0][0] = one<T>;
		m[1][1] = one<T>;
		m[2][2] = one<T>;
		m[3][3] = one<T>;
	}

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	void TMat4::initializeToPerspectiveMatrix(T FOV, T WHRatio, T zNear, T zFar)
	{
		m[0][0] = (one<T> / (tan(FOV / two<T>) * WHRatio));
		m[1][1] = (one<T> / tan(FOV / two<T>));
		m[2][2] = (-(zFar + zNear) / ((zFar - zNear)));
		m[2][3] = -one<T>;
		m[3][2] = (-(two<T> * zFar * zNear) / ((zFar - zNear)));
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	void TMat4::initializeToPerspectiveMatrix(T FOV, T WHRatio, T zNear, T zFar)
	{
		m[0][0] = (one<T> / (tan(FOV / two<T>) * WHRatio));
		m[1][1] = (one<T> / tan(FOV / two<T>));
		m[2][2] = (-(zFar + zNear) / ((zFar - zNear)));
		m[2][3] = (-(two<T> * zFar * zNear) / ((zFar - zNear)));
		m[3][2] = -one<T>;
	}
#endif

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	void TMat4::initializeToOrthographicMatrix(T left, T right, T bottom, T up, T zNear, T zFar)
	{
		m[0][0] = (two<T> / (right - left));
		m[1][1] = (two<T> / (up - bottom));
		m[2][2] = (-two<T> / (zFar - zNear));
		m[3][0] = (-(right + left) / (right - left));
		m[3][1] = (-(up + bottom) / (up - bottom));
		m[3][2] = (-(zFar + zNear) / (zFar - zNear));
		m[3][3] = one<T>;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	void TMat4::initializeToOrthographicMatrix(T left, T right, T bottom, T up, T zNear, T zFar)
	{
		m[0][0] = (two<T> / (right - left));
		m[0][3] = (-(right + left) / (right - left));
		m[1][1] = (two<T> / (up - bottom));
		m[1][3] = (-(up + bottom) / (up - bottom));
		m[2][2] = (-two<T> / (zFar - zNear));
		m[2][3] = (-(zFar + zNear) / (zFar - zNear));
		m[3][3] = one<T>;
	}
#endif
	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	auto TMat4::lookAt(const TVec4<T> & eyePos, const TVec4<T> & centerPos, const TVec4<T> & upDir) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;
		TVec4<T> l_X;
		TVec4<T> l_Y = upDir;
		TVec4<T> l_Z = TVec4<T>(centerPos.x - eyePos.x, centerPos.y - eyePos.y, centerPos.z - eyePos.z, T()).normalize();

		l_X = l_Z.cross(l_Y);
		l_X = l_X.normalize();
		l_Y = l_X.cross(l_Z);
		l_Y = l_Y.normalize();

		l_m.m[0][0] = l_X.x;
		l_m.m[0][1] = l_Y.x;
		l_m.m[0][2] = -l_Z.x;
		l_m.m[0][3] = T();
		l_m.m[1][0] = l_X.y;
		l_m.m[1][1] = l_Y.y;
		l_m.m[1][2] = -l_Z.y;
		l_m.m[1][3] = T();
		l_m.m[2][0] = l_X.z;
		l_m.m[2][1] = l_Y.z;
		l_m.m[2][2] = -l_Z.z;
		l_m.m[2][3] = T();
		l_m.m[3][0] = -(l_X * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[3][1] = -(l_Y * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[3][2] = (l_Z * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[3][3] = one<T>;

		return l_m;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	auto lookAt(const TVec4<T>& eyePos, const TVec4<T>& centerPos, const TVec4<T>& upDir) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;
		TVec4<T> l_X;
		TVec4<T> l_Y = upDir;
		TVec4<T> l_Z = TVec4<T>(centerPos.x - eyePos.x, centerPos.y - eyePos.y, centerPos.z - eyePos.z, T()).normalize();

		l_X = l_Z.cross(l_Y);
		l_X = l_X.normalize();
		l_Y = l_X.cross(l_Z);
		l_Y = l_Y.normalize();

		l_m.m[0][0] = l_X.x;
		l_m.m[0][1] = l_X.y;
		l_m.m[0][2] = l_X.z;
		l_m.m[0][3] = -(l_X * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[1][0] = l_Y.x;
		l_m.m[1][1] = l_Y.y;
		l_m.m[1][2] = l_Y.z;
		l_m.m[1][3] = -(l_Y * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[2][0] = -l_Z.x;
		l_m.m[2][1] = -l_Z.y;
		l_m.m[2][2] = -l_Z.z;
		l_m.m[2][3] = (l_Z * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[3][0] = T();
		l_m.m[3][1] = T();
		l_m.m[3][2] = T();
		l_m.m[3][3] = one<T>;

		return l_m;
	}
#endif
};

template<class T>
class TVertex
{
public:
	TVertex() :
		m_pos(TVec4<T>(T(), T(), T(), one<T>)),
		m_texCoord(TVec2<T>(T(), T())),
		m_normal(TVec4<T>(T(), T(), one<T>, T())) {};
	TVertex(const TVertex& rhs) :
		m_pos(rhs.m_pos),
		m_texCoord(rhs.m_texCoord),
		m_normal(rhs.m_normal) {};
	TVertex(const TVec4<T>& pos, const TVec2<T>& texCoord, const TVec4<T>& normal) :
		m_pos(pos),
		m_texCoord(texCoord),
		m_normal(normal) {};
	auto TVertex::operator=(const TVertex & rhs) -> TVertex<T>&
	{
		m_pos = rhs.m_pos;
		m_texCoord = rhs.m_texCoord;
		m_normal = rhs.m_normal;
		return *this;
	}
	~TVertex() {};

	TVec4<T> m_pos;
	TVec2<T> m_texCoord;
	TVec4<T> m_normal;
};

template<class T>
class TRay
{
public:
	TRay() :
		m_origin(TVec4<T>(T(), T(), T(), one<T>)),
		m_direction(TVec4<T>(T(), T(), T(), T())) {};
	TRay(const TRay<T>& rhs) :
		m_origin(rhs.m_origin),
		m_direction(rhs.m_direction) {};
	auto TRay::operator=(const TRay<T> & rhs) -> TRay<T>&
	{
		m_origin = rhs.m_origin;
		m_direction = rhs.m_direction;

		return *this;
	}
	~TRay() {};

	TVec4<T> m_origin;
	TVec4<T> m_direction;
};

template<class T>
class TAABB
{
public:
	TAABB() :
		m_center(TVec4<T>(T(), T(), T(), one<T>)),
		m_sphereRadius(T()),
		m_boundMin(TVec4<T>(T(), T(), T(), one<T>)),
		m_boundMax(TVec4<T>(T(), T(), T(), one<T>)) {};
	TAABB(const TAABB<T>& rhs) :
		m_center(rhs.m_center),
		m_sphereRadius(rhs.m_sphereRadius),
		m_boundMin(rhs.m_boundMin),
		m_boundMax(rhs.m_boundMax),
		m_vertices(rhs.m_vertices),
		m_indices(rhs.m_indices) {};
	auto TAABB::operator=(const TAABB<T> & rhs) -> TAABB<T>&
	{
		m_center = rhs.m_center;
		m_sphereRadius = rhs.m_sphereRadius;
		m_boundMin = rhs.m_boundMin;
		m_boundMax = rhs.m_boundMax;
		m_vertices = rhs.m_vertices;
		m_indices = rhs.m_indices;
		return *this;
	}
	~TAABB() {};

	TVec4<T> m_center;
	T m_sphereRadius;
	TVec4<T> m_boundMin;
	TVec4<T> m_boundMax;

	std::vector<TVertex<T>> m_vertices;
	std::vector<unsigned int> m_indices;
};

enum direction { FORWARD, BACKWARD, UP, DOWN, RIGHT, LEFT };

template<class T>
class TTransform
{
public:
	TTransform() :
		m_parentTransform(nullptr),
		m_pos(TVec4<T>(0.0, 0.0, 0.0, 1.0)),
		m_rot(TVec4<T>(0.0, 0.0, 0.0, 1.0)),
		m_scale(TVec4<T>(1.0, 1.0, 1.0, 1.0)),
		m_previousPos((m_pos + (1.0)) / 2.0),
		m_previousRot(m_rot.quatMul(TVec4<T>(1.0, 1.0, 1.0, 0.0))),
		m_previousScale(m_scale + (1.0)) {};
	~TTransform() {};

	bool TTransform::hasChanged()
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

	void TTransform::update()
	{
		m_previousPos = m_pos;
		m_previousRot = m_rot;
		m_previousScale = m_scale;
	}

	void TTransform::rotateInLocal(const TVec4<T> & axis, T angle)
	{
		T sinHalfAngle = std::sin((angle * PI<T> / halfCircumference<T>) / two<T>);
		T cosHalfAngle = std::cos((angle * PI<T> / halfCircumference<T>) / two<T>);
		// get final rotation
		m_rot = TVec4<T>(axis.x * sinHalfAngle, axis.y * sinHalfAngle, axis.z * sinHalfAngle, cosHalfAngle).quatMul(m_rot);
	}

	auto TTransform::getPos() -> TVec4<T>
	{
		return m_pos;
	}

	auto TTransform::getRot() -> TVec4<T>
	{
		return m_rot;
	}

	auto TTransform::getScale() -> TVec4<T>
	{
		return m_scale;
	}

	void TTransform::setLocalPos(const TVec4<T> & pos)
	{
		m_pos = pos;
	}

	void TTransform::setLocalRot(const TVec4<T> & rot)
	{
		m_rot = rot;
	}

	void TTransform::setLocalScale(const TVec4<T> & scale)
	{
		m_scale = scale;
	}

	auto TTransform::caclLocalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(m_pos);
	}

	auto TTransform::caclLocalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(m_rot);
	}

	auto TTransform::caclLocalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(m_scale);
	}

	auto TTransform::caclLocalTransformationMatrix() -> TMat4<T>
	{
		return caclLocalTranslationMatrix() * caclLocalRotMatrix() * caclLocalScaleMatrix();
	}

	auto TTransform::caclPreviousLocalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(m_previousPos);
	}

	auto TTransform::caclPreviousLocalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(m_previousRot);
	}

	auto TTransform::caclPreviousLocalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(m_previousScale);
	}

	auto TTransform::caclPreviousLocalTransformationMatrix() -> TMat4<T>
	{
		return caclPreviousLocalTranslationMatrix() * caclPreviousLocalRotMatrix() * caclPreviousLocalScaleMatrix();
	}

	auto TTransform::caclGlobalPos() -> TVec4<T>
	{
		TMat4<T> l_parentTransformationMatrix;
		l_parentTransformationMatrix.initializeToIdentityMatrix();

		if (nullptr != m_parentTransform)
		{
			l_parentTransformationMatrix = m_parentTransform->caclGlobalTransformationMatrix();
		}

		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(m_pos, l_parentTransformationMatrix);
		result = result * (one<T> / result.w);
		return result;
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(l_parentTransformationMatrix, m_pos);
		result = result * (one<T> / result.w);
		return result;
#endif
	}

	auto TTransform::caclGlobalRot() -> TVec4<T>
	{
		TVec4<T> l_parentRot = TVec4<T>(T(), T(), T(), one<T>);

		if (nullptr != m_parentTransform)
		{
			l_parentRot = m_parentTransform->caclGlobalRot();
		}

		return l_parentRot.quatMul(m_rot);
	}

	auto TTransform::caclGlobalScale() -> TVec4<T>
	{
		TVec4<T> l_parentScale = TVec4<T>(one<T>, one<T>, one<T>, one<T>);

		if (nullptr != m_parentTransform)
		{
			l_parentScale = m_parentTransform->caclGlobalScale();
		}

		return l_parentScale.scale(m_scale);
	}

	auto TTransform::caclPreviousGlobalPos() -> TVec4<T>
	{
		TMat4<T> l_parentTransformationMatrix;
		l_parentTransformationMatrix.initializeToIdentityMatrix();

		if (nullptr != m_parentTransform)
		{
			l_parentTransformationMatrix = m_parentTransform->caclPreviousGlobalTransformationMatrix();
		}

		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(m_previousPos, l_parentTransformationMatrix);
		result = result * (one<T> / result.w);
		return result;
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(l_parentTransformationMatrix, m_previousPos);
		result = result * (one<T> / result.w);
		return result;
#endif
	}

	auto TTransform::caclPreviousGlobalRot() -> TVec4<T>
	{
		TVec4<T> l_parentRot = TVec4<T>(T(), T(), T(), one<T>);

		if (nullptr != m_parentTransform)
		{
			l_parentRot = m_parentTransform->caclPreviousGlobalRot();
		}

		return l_parentRot.quatMul(m_previousRot);
	}

	auto TTransform::caclPreviousGlobalScale() -> TVec4<T>
	{
		TVec4<T> l_parentScale = TVec4<T>(one<T>, one<T>, one<T>, one<T>);

		if (nullptr != m_parentTransform)
		{
			l_parentScale = m_parentTransform->caclPreviousGlobalScale();
		}

		return l_parentScale.scale(m_previousScale);
	}

	auto TTransform::caclGlobalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(caclGlobalPos());
	}

	auto TTransform::caclGlobalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(caclGlobalRot());
	}

	auto TTransform::caclGlobalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(caclGlobalScale());
	}

	auto TTransform::caclGlobalTransformationMatrix() -> TMat4<T>
	{
		TMat4<T> l_parentTransformationMatrix;
		l_parentTransformationMatrix.initializeToIdentityMatrix();

		if (nullptr != m_parentTransform)
		{
			l_parentTransformationMatrix = m_parentTransform->caclGlobalTransformationMatrix();
		}

		return l_parentTransformationMatrix * caclLocalTransformationMatrix();
	}

	auto TTransform::caclPreviousGlobalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(caclPreviousGlobalPos());
	}

	auto TTransform::caclPreviousGlobalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(caclPreviousGlobalRot());
	}

	auto TTransform::caclPreviousGlobalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(caclPreviousGlobalScale());
	}

	auto TTransform::caclPreviousGlobalTransformationMatrix() -> TMat4<T>
	{
		TMat4<T> l_parentTransformationMatrix;
		l_parentTransformationMatrix.initializeToIdentityMatrix();

		if (nullptr != m_parentTransform)
		{
			l_parentTransformationMatrix = m_parentTransform->caclPreviousGlobalTransformationMatrix();
		}

		return l_parentTransformationMatrix * caclPreviousLocalTransformationMatrix();

	}

	auto TTransform::caclLookAtMatrix() -> TMat4<T>
	{
		return TMat4<T>().lookAt(caclGlobalPos(), caclGlobalPos() + getDirection(direction::BACKWARD), getDirection(direction::UP));
	}

	auto TTransform::caclPreviousLookAtMatrix() -> TMat4<T>
	{
		return TMat4<T>().lookAt(caclPreviousGlobalPos(), caclPreviousGlobalPos() + getPreviousDirection(direction::BACKWARD), getPreviousDirection(direction::UP));
	}

	auto TTransform::getInvertLocalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(m_pos.scale(-1.0));
	}

	auto TTransform::getInvertLocalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(m_rot.quatConjugate());
	}

	auto TTransform::getInvertLocalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(m_scale.reciprocal());
	}

	auto TTransform::getInvertGlobalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(caclGlobalPos().scale(-1.0));
	}

	auto TTransform::getInvertGlobalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(caclGlobalRot().quatConjugate());
	}

	auto TTransform::getInvertGlobalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(caclGlobalScale().reciprocal());
	}

	auto TTransform::getPreviousInvertLocalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(m_previousPos.scale(-1.0));
	}

	auto TTransform::getPreviousInvertLocalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(m_previousRot.quatConjugate());
	}

	auto TTransform::getPreviousInvertLocalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(m_previousScale.reciprocal());
	}

	auto TTransform::getPreviousInvertGlobalTranslationMatrix() -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(caclPreviousGlobalPos().scale(-1.0));
	}

	auto TTransform::getPreviousInvertGlobalRotMatrix() -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(caclPreviousGlobalRot().quatConjugate());
	}


	auto TTransform::getPreviousInvertGlobalScaleMatrix() -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(caclPreviousGlobalScale().reciprocal());
	}


	auto TTransform::getDirection(direction direction) -> TVec4<T>
	{
		TVec4<T> l_directionTVec4;

		switch (direction)
		{
		case FORWARD: l_directionTVec4 = TVec4<T>(0.0f, 0.0f, 1.0f, 0.0f); break;
		case BACKWARD:l_directionTVec4 = TVec4<T>(0.0f, 0.0f, -1.0f, 0.0f); break;
		case UP:l_directionTVec4 = TVec4<T>(0.0f, 1.0f, 0.0f, 0.0f); break;
		case DOWN:l_directionTVec4 = TVec4<T>(0.0f, -1.0f, 0.0f, 0.0f); break;
		case RIGHT:l_directionTVec4 = TVec4<T>(1.0f, 0.0f, 0.0f, 0.0f); break;
		case LEFT:l_directionTVec4 = TVec4<T>(-1.0f, 0.0f, 0.0f, 0.0f); break;
		}

		// V' = QVQ^-1, for unit quaternion, the conjugated quaternion is as same as the inverse quaternion

		// naive version
		// get Q * V by hand
		//TVec4 l_hiddenRotatedQuat;
		//l_hiddenRotatedQuat.w = -m_rot.x * l_directionTVec4.x - m_rot.y * l_directionTVec4.y - m_rot.z * l_directionTVec4.z;
		//l_hiddenRotatedQuat.x = m_rot.w * l_directionTVec4.x + m_rot.y * l_directionTVec4.z - m_rot.z * l_directionTVec4.y;
		//l_hiddenRotatedQuat.y = m_rot.w * l_directionTVec4.y + m_rot.z * l_directionTVec4.x - m_rot.x * l_directionTVec4.z;
		//l_hiddenRotatedQuat.z = m_rot.w * l_directionTVec4.z + m_rot.x * l_directionTVec4.y - m_rot.y * l_directionTVec4.x;

		// get conjugated quaternion
		//TVec4 l_conjugatedQuat;
		//l_conjugatedQuat = conjugate(m_rot);

		// then QV * Q^-1 
		//TVec4 l_directionQuat;
		//l_directionQuat = l_hiddenRotatedQuat * l_conjugatedQuat;
		//l_directionTVec4.x = l_directionQuat.x;
		//l_directionTVec4.y = l_directionQuat.y;
		//l_directionTVec4.z = l_directionQuat.z;

		// traditional version, change direction vector to quaternion representation

		//TVec4 l_directionQuat = TVec4(0.0, l_directionTVec4);
		//l_directionQuat = m_rot * l_directionQuat * conjugate(m_rot);
		//l_directionTVec4.x = l_directionQuat.x;
		//l_directionTVec4.y = l_directionQuat.y;
		//l_directionTVec4.z = l_directionQuat.z;

		// optimized version ([Kavan et al. ] Lemma 4)
		//V' = V + 2 * Qv x (Qv x V + Qs * V)
		TVec4<T> l_Qv = TVec4<T>(m_rot.x, m_rot.y, m_rot.z, m_rot.w);
		l_directionTVec4 = l_directionTVec4 + l_Qv.cross((l_Qv.cross(l_directionTVec4) + l_directionTVec4.scale(m_rot.w))).scale(2.0f);

		return l_directionTVec4.normalize();
	}


	auto TTransform::getPreviousDirection(direction direction) -> TVec4<T>
	{
		TVec4<T> l_directionTVec4;

		switch (direction)
		{
		case FORWARD: l_directionTVec4 = TVec4<T>(0.0f, 0.0f, 1.0f, 0.0f); break;
		case BACKWARD:l_directionTVec4 = TVec4<T>(0.0f, 0.0f, -1.0f, 0.0f); break;
		case UP:l_directionTVec4 = TVec4<T>(0.0f, 1.0f, 0.0f, 0.0f); break;
		case DOWN:l_directionTVec4 = TVec4<T>(0.0f, -1.0f, 0.0f, 0.0f); break;
		case RIGHT:l_directionTVec4 = TVec4<T>(1.0f, 0.0f, 0.0f, 0.0f); break;
		case LEFT:l_directionTVec4 = TVec4<T>(-1.0f, 0.0f, 0.0f, 0.0f); break;
		}

		// V' = QVQ^-1, for unit quaternion, the conjugated quaternion is as same as the inverse quaternion

		// naive version
		// get Q * V by hand
		//TVec4 l_hiddenRotatedQuat;
		//l_hiddenRotatedQuat.w = -m_rot.x * l_directionTVec4.x - m_rot.y * l_directionTVec4.y - m_rot.z * l_directionTVec4.z;
		//l_hiddenRotatedQuat.x = m_rot.w * l_directionTVec4.x + m_rot.y * l_directionTVec4.z - m_rot.z * l_directionTVec4.y;
		//l_hiddenRotatedQuat.y = m_rot.w * l_directionTVec4.y + m_rot.z * l_directionTVec4.x - m_rot.x * l_directionTVec4.z;
		//l_hiddenRotatedQuat.z = m_rot.w * l_directionTVec4.z + m_rot.x * l_directionTVec4.y - m_rot.y * l_directionTVec4.x;

		// get conjugated quaternion
		//TVec4 l_conjugatedQuat;
		//l_conjugatedQuat = conjugate(m_rot);

		// then QV * Q^-1 
		//TVec4 l_directionQuat;
		//l_directionQuat = l_hiddenRotatedQuat * l_conjugatedQuat;
		//l_directionTVec4.x = l_directionQuat.x;
		//l_directionTVec4.y = l_directionQuat.y;
		//l_directionTVec4.z = l_directionQuat.z;

		// traditional version, change direction vector to quaternion representation

		//TVec4 l_directionQuat = TVec4(0.0, l_directionTVec4);
		//l_directionQuat = m_rot * l_directionQuat * conjugate(m_rot);
		//l_directionTVec4.x = l_directionQuat.x;
		//l_directionTVec4.y = l_directionQuat.y;
		//l_directionTVec4.z = l_directionQuat.z;

		// optimized version ([Kavan et al. ] Lemma 4)
		//V' = V + 2 * Qv x (Qv x V + Qs * V)
		TVec4<T> l_Qv = TVec4<T>(m_previousRot.x, m_previousRot.y, m_previousRot.z, m_previousRot.w);
		l_directionTVec4 = l_directionTVec4 + l_Qv.cross((l_Qv.cross(l_directionTVec4) + l_directionTVec4.scale(m_previousRot.w))).scale(2.0f);

		return l_directionTVec4.normalize();
	}
	TTransform* m_parentTransform;

private:
	TVec4<T> m_pos;
	TVec4<T> m_rot;
	TVec4<T> m_scale;

	TVec4<T> m_previousPos;
	TVec4<T> m_previousRot;
	TVec4<T> m_previousScale;
};

#if defined (INNO_RENDERER_OPENGL)
using vec2 = TVec2<double>;
using vec4 = TVec4<double>;
using mat4 = TMat4<double>;
using Vertex = TVertex<double>;
using Ray = TRay<double>;
using AABB = TAABB<double>;
using Transform = TTransform<double>;
#elif defined (INNO_RENDERER_DX)
using vec2 = TVec2<float>;
using vec4 = TVec4<float>;
using mat4 = TMat4<float>;
using Vertex = TVertex<float>;
using Ray = TRay<float>;
using AABB = TAABB<float>;
using Transform = TTransform<float>;
#endif

namespace InnoMath
{
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto mul(const TVec4<T> & lhs, const TMat4<T> & rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_TVec4;

		l_TVec4.x = lhs.x * rhs.m[0][0] + lhs.y * rhs.m[1][0] + lhs.z * rhs.m[2][0] + lhs.w * rhs.m[3][0];
		l_TVec4.y = lhs.x * rhs.m[0][1] + lhs.y * rhs.m[1][1] + lhs.z * rhs.m[2][1] + lhs.w * rhs.m[3][1];
		l_TVec4.z = lhs.x * rhs.m[0][2] + lhs.y * rhs.m[1][2] + lhs.z * rhs.m[2][2] + lhs.w * rhs.m[3][2];
		l_TVec4.w = lhs.x * rhs.m[0][3] + lhs.y * rhs.m[1][3] + lhs.z * rhs.m[2][3] + lhs.w * rhs.m[3][3];

		return l_TVec4;
	}
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto mul(const TMat4<T> & lhs, const TVec4<T> & rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_TVec4;

		l_TVec4.x = lhs.m[0][0] * rhs.x + lhs.m[0][1] * rhs.y + lhs.m[0][2] * rhs.z + lhs.m[0][3] * rhs.w;
		l_TVec4.y = lhs.m[1][0] * rhs.x + lhs.m[1][1] * rhs.y + lhs.m[1][2] * rhs.z + lhs.m[1][3] * rhs.w;
		l_TVec4.z = lhs.m[2][0] * rhs.x + lhs.m[2][1] * rhs.y + lhs.m[2][2] * rhs.z + lhs.m[2][3] * rhs.w;
		l_TVec4.w = lhs.m[3][0] * rhs.x + lhs.m[3][1] * rhs.y + lhs.m[3][2] * rhs.z + lhs.m[3][3] * rhs.w;

		return l_TVec4;
	}
#endif
	/*
	Column-Major memory layout and
	Row-Major vector4 mathematical convention

	vector4(a matrix1x4) :
	| x y z w |

	matrix4x4 £º
	[columnIndex][rowIndex]
	| m[0][0] <-> a00(1.0) m[1][0] <->  a01(0.0) m[2][0] <->  a02(0.0) m[3][0] <->  a03(0.0) |
	| m[0][1] <-> a10(0.0) m[1][1] <->  a11(1.0) m[2][1] <->  a12(0.0) m[3][1] <->  a13(0.0) |
	| m[0][2] <-> a20(0.0) m[1][2] <->  a21(0.0) m[2][2] <->  a22(1.0) m[3][2] <->  a23(0.0) |
	| m[0][3] <-> a30(Tx ) m[1][3] <->  a31(Ty ) m[2][3] <->  a32(Tz ) m[3][3] <->  a33(1.0) |

	in

	vector4 * matrix4x4 :
	| x' = x * a00(1.0) + y * a10(0.0) + z * a20(0.0) + w * a30 (Tx) |
	| y' = x * a01(0.0) + y * a11(1.0) + z * a21(0.0) + w * a31 (Ty) |
	| z' = x * a02(0.0) + y * a12(0.0) + z * a22(1.0) + w * a32 (Tz) |
	| w' = x * a03(0.0) + y * a13(0.0) + z * a23(0.0) + w * a33(1.0) |

	--------------------------------------------

	Row-Major memory layout and
	Column-Major vector4 mathematical convention

	vector4(a matrix4x1) :
	| x |
	| y |
	| z |
	| w |

	matrix4x4 £º
	[rowIndex][columnIndex]
	| m[0][0] <-> a00(1.0) m[0][1] <->  a01(0.0) m[0][2] <->  a02(0.0) m[0][3] <->  a03(Tx ) |
	| m[1][0] <-> a10(0.0) m[1][1] <->  a11(1.0) m[1][2] <->  a12(0.0) m[1][3] <->  a13(Ty ) |
	| m[2][0] <-> a20(0.0) m[2][1] <->  a21(0.0) m[2][2] <->  a22(1.0) m[2][3] <->  a23(Tz ) |
	| m[3][0] <-> a30(0.0) m[3][1] <->  a31(0.0) m[3][2] <->  a32(0.0) m[3][3] <->  a33(1.0) |

	in

	matrix4x4 * vector4 :
	| x' = a00(1.0) * x  + a01(0.0) * y + a02(0.0) * z + a03(Tx ) * w |
	| y' = a10(0.0) * x  + a11(1.0) * y + a12(0.0) * z + a13(Ty ) * w |
	| z' = a20(0.0) * x  + a21(0.0) * y + a22(1.0) * z + a23(Tz ) * w |
	| w' = a30(0.0) * x  + a31(0.0) * y + a32(0.0) * z + a33(1.0) * w |
	*/

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto TVec4::toTranslationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m[0][0] = one<T>;
		l_m.m[1][1] = one<T>;
		l_m.m[2][2] = one<T>;
		l_m.m[3][0] = rhs.x;
		l_m.m[3][1] = rhs.y;
		l_m.m[3][2] = rhs.z;
		l_m.m[3][3] = one<T>;

		return l_m;
	}
	//Row-Major memory layout
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toTranslationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m[0][0] = one<T>;
		l_m.m[0][3] = rhs.x;
		l_m.m[1][1] = one<T>;
		l_m.m[1][3] = rhs.y;
		l_m.m[2][2] = one<T>;
		l_m.m[2][3] = rhs.z;
		l_m.m[3][3] = one<T>;

		return l_m;
	}
#endif

	/*
	Column-Major memory layout and
	Row-Major vector4 mathematical convention

	vector4(a matrix1x4) :
	| x y z w |

	matrix4x4 £º
	[columnIndex][rowIndex]
	| m[0][0] <-> a00(1 - 2*qy2 - 2*qz2) m[1][0] <->  a01(2*qx*qy + 2*qz*qw) m[2][0] <->  a02(2*qx*qz - 2*qy*qw) m[3][0] <->  a03(0.0) |
	| m[0][1] <-> a10(2*qx*qy - 2*qz*qw) m[1][1] <->  a11(1 - 2*qx2 - 2*qz2) m[2][1] <->  a12(2*qy*qz + 2*qx*qw) m[3][1] <->  a13(0.0) |
	| m[0][2] <-> a20(2*qx*qz + 2*qy*qw) m[1][2] <->  a21(2*qy*qz - 2*qx*qw) m[2][2] <->  a22(1 - 2*qx2 - 2*qy2) m[3][2] <->  a23(0.0) |
	| m[0][3] <-> a30(       0.0       ) m[1][3] <->  a31(       0.0       ) m[2][3] <->  a32(       0.0       ) m[3][3] <->  a33(1.0) |

	in

	vector4 * matrix4x4 :
	| x' = x * a00(1 - 2*qy2 - 2*qz2) + y * a10(2*qx*qy - 2*qz*qw) + z * a20(2*qx*qz + 2*qy*qw) + w * a30(       0.0       ) |
	| y' = x * a01(2*qx*qy + 2*qz*qw) + y * a11(1 - 2*qx2 - 2*qz2) + z * a21(2*qy*qz - 2*qx*qw) + w * a31(       0.0       ) |
	| z' = x * a02(2*qx*qz - 2*qy*qw) + y * a12(2*qy*qz + 2*qx*qw) + z * a22(1 - 2*qx2 - 2*qy2) + w * a32(       0.0       ) |
	| w' = x * a03(       0.0       ) + y * a13(       0.0       ) + z * a23(       0.0       ) + w * a33(       1.0       ) |

	--------------------------------------------

	Row-Major memory layout and
	Column-Major vector4 mathematical convention

	vector4(a matrix4x1) :
	| x |
	| y |
	| z |
	| w |

	matrix4x4 £º
	[rowIndex][columnIndex]
	| m[0][0] <-> a00(1 - 2*qy2 - 2*qz2) m[0][1] <->  a01(2*qx*qy - 2*qz*qw) m[0][2] <->  a02(2*qx*qz + 2*qy*qw) m[0][3] <->  a03(0.0) |
	| m[1][0] <-> a10(2*qx*qy + 2*qz*qw) m[1][1] <->  a11(1 - 2*qx2 - 2*qz2) m[1][2] <->  a12(2*qy*qz - 2*qx*qw) m[1][3] <->  a13(0.0) |
	| m[2][0] <-> a20(2*qx*qz - 2*qy*qw) m[2][1] <->  a21(2*qy*qz + 2*qx*qw) m[2][2] <->  a22(1 - 2*qx2 - 2*qy2) m[2][3] <->  a23(0.0) |
	| m[3][0] <-> a30(       0.0       ) m[3][1] <->  a31(       0.0       ) m[3][2] <->  a32(       0.0       ) m[3][3] <->  a33(1.0) |

	in

	matrix4x4 * vector4 :
	| x' = a00(1 - 2*qy2 - 2*qz2) * x  + a01(2*qx*qy - 2*qz*qw) * y + a02(2*qx*qz + 2*qy*qw) * z + a03(       0.0       ) * w |
	| y' = a10(2*qx*qy + 2*qz*qw) * x  + a11(1 - 2*qx2 - 2*qz2) * y + a12(2*qy*qz - 2*qx*qw) * z + a13(       0.0       ) * w |
	| z' = a20(2*qx*qz - 2*qy*qw) * x  + a21(2*qy*qz + 2*qx*qw) * y + a22(1 - 2*qx2 - 2*qy2) * z + a23(       0.0       ) * w |
	| w' = a30(       0.0       ) * x  + a31(       0.0       ) * y + a32(       0.0       ) * z + a33(       1.0       ) * w |
	*/

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toRotationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m[0][0] = (one<T> -two<T> * rhs.y * rhs.y - two<T> * rhs.z * rhs.z);
		l_m.m[0][1] = (two<T> * rhs.x * y + two<T> * rhs.z * rhs.w);
		l_m.m[0][2] = (two<T> * rhs.x * rhs.z - two<T> * rhs.y * rhs.w);
		l_m.m[0][3] = (T());

		l_m.m[1][0] = (two<T> * rhs.x * rhs.y - two<T> * rhs.z * rhs.w);
		l_m.m[1][1] = (one<T> -two<T> * rhs.x * rhs.x - two<T> * rhs.z * rhs.z);
		l_m.m[1][2] = (two<T> * rhs.y * rhs.z + two<T> * rhs.x * rhs.w);
		l_m.m[1][3] = (T());

		l_m.m[2][0] = (two<T> * rhs.x * rhs.z + two<T> * rhs.y * rhs.w);
		l_m.m[2][1] = (two<T> * rhs.y * rhs.z - two<T> * rhs.x * rhs.w);
		l_m.m[2][2] = (one<T> -two<T> * rhs.x * rhs.x - two<T> * rhs.y * rhs.y);
		l_m.m[2][3] = (T());

		l_m.m[3][0] = (T());
		l_m.m[3][1] = (T());
		l_m.m[3][2] = (T());
		l_m.m[3][3] = (one<T>);

		return l_m;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toRotationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m[0][0] = (one<T> -two<T> *  rhs.y *  rhs.y - two<T> *  rhs.z *  rhs.z);
		l_m.m[0][1] = (two<T> *  rhs.x *  rhs.y - two<T> *  rhs.z *  rhs.w);
		l_m.m[0][2] = (two<T> *  rhs.x *  rhs.z + two<T> *  rhs.y *  rhs.w);
		l_m.m[0][3] = (T());

		l_m.m[1][0] = (two<T> *  rhs.x *  rhs.y + two<T> *  rhs.z *  rhs.w);
		l_m.m[1][1] = (one<T> -two<T> *  rhs.x *  rhs.x - two<T> *  rhs.z *  rhs.z);
		l_m.m[1][2] = (two<T> *  rhs.y *  rhs.z - two<T> *  rhs.x *  rhs.w);
		l_m.m[1][3] = (T());

		l_m.m[2][0] = (two<T> *  rhs.x *  rhs.z - two<T> *  rhs.y *  rhs.w);
		l_m.m[2][1] = (two<T> *  rhs.y *  rhs.z + two<T> *  rhs.x *  rhs.w);
		l_m.m[2][2] = (one<T> -two<T> *  rhs.x *  rhs.x - two<T> *  rhs.y *  rhs.y);
		l_m.m[2][3] = (T());

		l_m.m[3][0] = (T());
		l_m.m[3][1] = (T());
		l_m.m[3][2] = (T());
		l_m.m[3][3] = one<T>;

		return l_m;
	}
#endif

	template<class T>
	auto toScaleMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;
		l_m.m[0][0] = rhs.x;
		l_m.m[1][1] = rhs.y;
		l_m.m[2][2] = rhs.z;
		l_m.m[3][3] = rhs.w;
		return l_m;
	}

	template<class T>
	bool intersectCheck(const TAABB<T> & lhs, const TAABB<T> & rhs)
	{
		if (rhs.m_center.x - lhs.m_center.x > rhs.m_sphereRadius + lhs.m_sphereRadius)
		{
			return false;
		}
		if (rhs.m_center.y - lhs.m_center.y > rhs.m_sphereRadius + lhs.m_sphereRadius)
		{
			return false;
		}
		if (rhs.m_center.z - lhs.m_center.z > rhs.m_sphereRadius + lhs.m_sphereRadius)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	template<class T>
	bool intersectCheck(const TAABB<T> & lhs, const TRay<T> & rhs)
	{
		T txmin, txmax, tymin, tymax, tzmin, tzmax;
		T l_invDirectionX = std::isinf(one<T> / rhs.m_direction.x) ? zero<T> : one<T> / rhs.m_direction.x;
		T l_invDirectionY = std::isinf(one<T> / rhs.m_direction.y) ? zero<T> : one<T> / rhs.m_direction.y;
		T l_invDirectionZ = std::isinf(one<T> / rhs.m_direction.z) ? zero<T> : one<T> / rhs.m_direction.z;
		TVec4<T> l_invDirection = TVec4<T>(l_invDirectionX, l_invDirectionY, l_invDirectionZ, zero<T>).normalize();

		if (l_invDirection.x >= zero<T>)
		{
			txmin = (lhs.m_boundMin.x - rhs.m_origin.x) * l_invDirection.x;
			txmax = (lhs.m_boundMax.x - rhs.m_origin.x) * l_invDirection.x;
		}
		else
		{
			txmin = (lhs.m_boundMax.x - rhs.m_origin.x) * l_invDirection.x;
			txmax = (lhs.m_boundMin.x - rhs.m_origin.x) * l_invDirection.x;
		}
		if (l_invDirection.y >= zero<T>)
		{
			tymin = (lhs.m_boundMin.y - rhs.m_origin.y) * l_invDirection.y;
			tymax = (lhs.m_boundMax.y - rhs.m_origin.y) * l_invDirection.y;
		}
		else
		{
			tymin = (lhs.m_boundMax.y - rhs.m_origin.y) * l_invDirection.y;
			tymax = (lhs.m_boundMin.y - rhs.m_origin.y) * l_invDirection.y;
		}
		if (txmin > tymax || tymin > txmax)
		{
			return false;
		}
		if (l_invDirection.z >= zero<T>)
		{
			tzmin = (lhs.m_boundMin.z - rhs.m_origin.z) * l_invDirection.z;
			tzmax = (lhs.m_boundMax.z - rhs.m_origin.z) * l_invDirection.z;
		}
		else {
			tzmin = (lhs.m_boundMax.z - rhs.m_origin.z) * l_invDirection.z;
			tzmax = (lhs.m_boundMin.z - rhs.m_origin.z) * l_invDirection.z;
		}
		if (txmin > tzmax || tzmin > txmax)
		{
			return false;
		}
		txmin = (tymin > txmin) || std::isinf(txmin) ? tymin : txmin;
		txmax = (tymax < txmax) || std::isinf(txmax) ? tymax : txmax;
		txmin = (tzmin > txmin) ? tzmin : txmin;
		txmax = (tzmax < txmax) ? tzmax : txmax;
		if (txmin < txmax && txmax >= zero<T>)
		{
			return true;
		}
		return false;
	}

	template<class T, class U>
	auto precisionConvert(TMat4<T> rhs) ->TMat4<U>
	{
		TMat4<U> l_m;

		l_m.m[0][0] = static_cast<U>(rhs.m[0][0]);
		l_m.m[0][1] = static_cast<U>(rhs.m[0][1]);
		l_m.m[0][2] = static_cast<U>(rhs.m[0][2]);
		l_m.m[0][3] = static_cast<U>(rhs.m[0][3]);
		l_m.m[1][0] = static_cast<U>(rhs.m[1][0]);
		l_m.m[1][1] = static_cast<U>(rhs.m[1][1]);
		l_m.m[1][2] = static_cast<U>(rhs.m[1][2]);
		l_m.m[1][3] = static_cast<U>(rhs.m[1][3]);
		l_m.m[2][0] = static_cast<U>(rhs.m[2][0]);
		l_m.m[2][1] = static_cast<U>(rhs.m[2][1]);
		l_m.m[2][2] = static_cast<U>(rhs.m[2][2]);
		l_m.m[2][3] = static_cast<U>(rhs.m[2][3]);
		l_m.m[3][0] = static_cast<U>(rhs.m[3][0]);
		l_m.m[3][1] = static_cast<U>(rhs.m[3][1]);
		l_m.m[3][2] = static_cast<U>(rhs.m[3][2]);
		l_m.m[3][3] = static_cast<U>(rhs.m[3][3]);

		return l_m;
	}
}
