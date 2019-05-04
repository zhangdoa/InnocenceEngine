#pragma once
#include "../common/stl14.h"
#include "../common/config.h"
#include "../common/InnoType.h"

//typedef __m128 TVec4;

#undef max
#undef min

template<class T>
const static T PI = T(3.14159265358979323846264338327950288L);
template<class T>
const static T E = T(2.71828182845904523536028747135266249L);

template<class T>
T epsilon2 = T(0.01L);

template<class T>
T epsilon4 = T(0.0001L);

template<class T>
T epsilon8 = T(0.00000001L);

template<class T>
T zero = T(0.0L);

template<class T>
T half = T(0.5L);

template<class T>
T one = T(1.0L);

template<class T>
T two = T(2.0L);

template<class T>
T halfCircumference = T(180.0L);

template<class T>
T fullCircumference = T(360.0L);

template<class T>
class TVec2
{
public:
	T x;
	T y;

	TVec2() noexcept
	{
		x = T();
		y = T();
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

	TVec4() noexcept
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

	auto rotateDirectionByQuat(const TVec4<T> & rhs) -> TVec4<T>
	{
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
		auto result = *this + rhs.cross(rhs.cross(*this) + *this * rhs.w) * two<T>;

		return result.normalize();
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
| m00 <-> a00 m10 <-> a01 m20 <-> a02 m30 <-> a03 |
| m01 <-> a10 m11 <-> a11 m21 <-> a12 m31 <-> a13 |
| m02 <-> a20 m12 <-> a21 m22 <-> a22 m32 <-> a23 |
| m03 <-> a30 m13 <-> a31 m23 <-> a32 m33 <-> a33 |
vector4 :
m00 <-> x m10 <-> y m20 <-> z m30 <-> w
*/

/* Row-Major memory layout (in C/C++)
matrix4x4 :
[rowIndex][columnIndex]
| m00 <-> a00 m01 <-> a01 m02 <-> a02 m03 <-> a03 |
| m10 <-> a10 m11 <-> a11 m12 <-> a12 m13 <-> a13 |
| m20 <-> a20 m21 <-> a21 m22 <-> a22 m23 <-> a23 |
| m30 <-> a30 m31 <-> a31 m32 <-> a32 m33 <-> a33 |
vector4 :
m00 <-> x m01 <-> y m02 <-> z m03 <-> w  (best choice)
*/

template<class T>
class TMat4
{
public:
	T m00 = T();
	T m01 = T();
	T m02 = T();
	T m03 = T();
	T m10 = T();
	T m11 = T();
	T m12 = T();
	T m13 = T();
	T m20 = T();
	T m21 = T();
	T m22 = T();
	T m23 = T();
	T m30 = T();
	T m31 = T();
	T m32 = T();
	T m33 = T();

	TMat4() noexcept
	{
		m00 = T();
		m01 = T();
		m02 = T();
		m03 = T();
		m10 = T();
		m11 = T();
		m12 = T();
		m13 = T();
		m20 = T();
		m21 = T();
		m22 = T();
		m23 = T();
		m30 = T();
		m31 = T();
		m32 = T();
		m33 = T();
	}
	TMat4(const TMat4<T>& rhs)
	{
		m00 = rhs.m00;
		m01 = rhs.m01;
		m02 = rhs.m02;
		m03 = rhs.m03;
		m10 = rhs.m10;
		m11 = rhs.m11;
		m12 = rhs.m12;
		m13 = rhs.m13;
		m20 = rhs.m20;
		m21 = rhs.m21;
		m22 = rhs.m22;
		m23 = rhs.m23;
		m30 = rhs.m30;
		m31 = rhs.m31;
		m32 = rhs.m32;
		m33 = rhs.m33;
	}
	auto operator=(const TMat4<T>& rhs) -> TMat4<T>&
	{
		m00 = rhs.m00;
		m01 = rhs.m01;
		m02 = rhs.m02;
		m03 = rhs.m03;
		m10 = rhs.m10;
		m11 = rhs.m11;
		m12 = rhs.m12;
		m13 = rhs.m13;
		m20 = rhs.m20;
		m21 = rhs.m21;
		m22 = rhs.m22;
		m23 = rhs.m23;
		m30 = rhs.m30;
		m31 = rhs.m31;
		m32 = rhs.m32;
		m33 = rhs.m33;

		return *this;
	}
	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	auto TMat4::operator*(const TMat4<T> & rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = m00 * rhs.m00 + m10 * rhs.m01 + m20 * rhs.m02 + m30 * rhs.m03;
		l_m.m01 = m01 * rhs.m00 + m11 * rhs.m01 + m21 * rhs.m02 + m31 * rhs.m03;
		l_m.m02 = m02 * rhs.m00 + m12 * rhs.m01 + m22 * rhs.m02 + m32 * rhs.m03;
		l_m.m03 = m03 * rhs.m00 + m13 * rhs.m01 + m23 * rhs.m02 + m33 * rhs.m03;

		l_m.m10 = m00 * rhs.m10 + m10 * rhs.m11 + m20 * rhs.m12 + m30 * rhs.m13;
		l_m.m11 = m01 * rhs.m10 + m11 * rhs.m11 + m21 * rhs.m12 + m31 * rhs.m13;
		l_m.m12 = m02 * rhs.m10 + m12 * rhs.m11 + m22 * rhs.m12 + m32 * rhs.m13;
		l_m.m13 = m03 * rhs.m10 + m13 * rhs.m11 + m23 * rhs.m12 + m33 * rhs.m13;

		l_m.m20 = m00 * rhs.m20 + m10 * rhs.m21 + m20 * rhs.m22 + m30 * rhs.m23;
		l_m.m21 = m01 * rhs.m20 + m11 * rhs.m21 + m21 * rhs.m22 + m31 * rhs.m23;
		l_m.m22 = m02 * rhs.m20 + m12 * rhs.m21 + m22 * rhs.m22 + m32 * rhs.m23;
		l_m.m23 = m03 * rhs.m20 + m13 * rhs.m21 + m23 * rhs.m22 + m33 * rhs.m23;

		l_m.m30 = m00 * rhs.m30 + m10 * rhs.m31 + m20 * rhs.m32 + m30 * rhs.m33;
		l_m.m31 = m01 * rhs.m30 + m11 * rhs.m31 + m21 * rhs.m32 + m31 * rhs.m33;
		l_m.m32 = m02 * rhs.m30 + m12 * rhs.m31 + m22 * rhs.m32 + m32 * rhs.m33;
		l_m.m33 = m03 * rhs.m30 + m13 * rhs.m31 + m23 * rhs.m32 + m33 * rhs.m33;

		return l_m;
	}
	//Row-Major memory layout
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	auto operator*(const TMat4<T> & rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = m00 * rhs.m00 + m01 * rhs.m10 + m02 * rhs.m20 + m03 * rhs.m30;
		l_m.m01 = m00 * rhs.m01 + m01 * rhs.m11 + m02 * rhs.m21 + m03 * rhs.m31;
		l_m.m02 = m00 * rhs.m02 + m01 * rhs.m12 + m02 * rhs.m22 + m03 * rhs.m32;
		l_m.m03 = m00 * rhs.m03 + m01 * rhs.m13 + m02 * rhs.m23 + m03 * rhs.m33;

		l_m.m10 = m10 * rhs.m00 + m11 * rhs.m10 + m12 * rhs.m20 + m13 * rhs.m30;
		l_m.m11 = m10 * rhs.m01 + m11 * rhs.m11 + m12 * rhs.m21 + m13 * rhs.m31;
		l_m.m12 = m10 * rhs.m02 + m11 * rhs.m12 + m12 * rhs.m22 + m13 * rhs.m32;
		l_m.m13 = m10 * rhs.m03 + m11 * rhs.m13 + m12 * rhs.m23 + m13 * rhs.m33;

		l_m.m20 = m20 * rhs.m00 + m21 * rhs.m10 + m22 * rhs.m20 + m23 * rhs.m30;
		l_m.m21 = m20 * rhs.m01 + m21 * rhs.m11 + m22 * rhs.m21 + m23 * rhs.m31;
		l_m.m22 = m20 * rhs.m02 + m21 * rhs.m12 + m22 * rhs.m22 + m23 * rhs.m32;
		l_m.m23 = m20 * rhs.m03 + m21 * rhs.m13 + m22 * rhs.m23 + m23 * rhs.m33;

		l_m.m30 = m30 * rhs.m00 + m31 * rhs.m10 + m32 * rhs.m20 + m33 * rhs.m30;
		l_m.m31 = m30 * rhs.m01 + m31 * rhs.m11 + m32 * rhs.m21 + m33 * rhs.m31;
		l_m.m32 = m30 * rhs.m02 + m31 * rhs.m12 + m32 * rhs.m22 + m33 * rhs.m32;
		l_m.m33 = m30 * rhs.m03 + m31 * rhs.m13 + m32 * rhs.m23 + m33 * rhs.m33;

		return l_m;
	}
#endif

	auto operator*(const T rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = rhs * m00;
		l_m.m01 = rhs * m01;
		l_m.m02 = rhs * m02;
		l_m.m03 = rhs * m03;
		l_m.m10 = rhs * m10;
		l_m.m11 = rhs * m11;
		l_m.m12 = rhs * m12;
		l_m.m13 = rhs * m13;
		l_m.m20 = rhs * m20;
		l_m.m21 = rhs * m21;
		l_m.m22 = rhs * m22;
		l_m.m23 = rhs * m23;
		l_m.m30 = rhs * m30;
		l_m.m31 = rhs * m31;
		l_m.m32 = rhs * m32;
		l_m.m33 = rhs * m33;

		return l_m;
	}

	auto transpose() -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = m00;
		l_m.m01 = m10;
		l_m.m02 = m20;
		l_m.m03 = m30;
		l_m.m10 = m01;
		l_m.m11 = m11;
		l_m.m12 = m21;
		l_m.m13 = m31;
		l_m.m20 = m02;
		l_m.m21 = m12;
		l_m.m22 = m22;
		l_m.m23 = m32;
		l_m.m30 = m03;
		l_m.m31 = m13;
		l_m.m32 = m23;
		l_m.m33 = m33;

		return l_m;
	}
	auto inverse() -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = m21 * m32 * m13 - m31 * m22 * m13 + m31 * m12 * m23 - m11 * m32 * m23 - m21 * m12 * m33 + m11 * m22 * m33;
		l_m.m10 = m30 * m22 * m13 - m20 * m32 * m13 - m30 * m12 * m23 + m10 * m32 * m23 + m20 * m12 * m33 - m10 * m22 * m33;
		l_m.m20 = m20 * m31 * m13 - m30 * m21 * m13 + m30 * m11 * m23 - m10 * m31 * m23 - m20 * m11 * m33 + m10 * m21 * m33;
		l_m.m30 = m30 * m21 * m12 - m20 * m31 * m12 - m30 * m11 * m22 + m10 * m31 * m22 + m20 * m11 * m32 - m10 * m21 * m32;
		l_m.m01 = m31 * m22 * m03 - m21 * m32 * m03 - m31 * m02 * m23 + m01 * m32 * m23 + m21 * m02 * m33 - m01 * m22 * m33;
		l_m.m11 = m20 * m32 * m03 - m30 * m22 * m03 + m30 * m02 * m23 - m00 * m32 * m23 - m20 * m02 * m33 + m00 * m22 * m33;
		l_m.m21 = m30 * m21 * m03 - m20 * m31 * m03 - m30 * m01 * m23 + m00 * m31 * m23 + m20 * m01 * m33 - m00 * m21 * m33;
		l_m.m31 = m20 * m31 * m02 - m30 * m21 * m02 + m30 * m01 * m22 - m00 * m31 * m22 - m20 * m01 * m32 + m00 * m21 * m32;
		l_m.m02 = m11 * m32 * m03 - m31 * m12 * m03 + m31 * m02 * m13 - m01 * m32 * m13 - m11 * m02 * m33 + m01 * m12 * m33;
		l_m.m12 = m30 * m12 * m03 - m10 * m32 * m03 - m30 * m02 * m13 + m00 * m32 * m13 + m10 * m02 * m33 - m00 * m12 * m33;
		l_m.m22 = m10 * m31 * m03 - m30 * m11 * m03 + m30 * m01 * m13 - m00 * m31 * m13 - m10 * m01 * m33 + m00 * m11 * m33;
		l_m.m32 = m30 * m11 * m02 - m10 * m31 * m02 - m30 * m01 * m12 + m00 * m31 * m12 + m10 * m01 * m32 - m00 * m11 * m32;
		l_m.m03 = m21 * m12 * m03 - m11 * m22 * m03 - m21 * m02 * m13 + m01 * m22 * m13 + m11 * m02 * m23 - m01 * m12 * m23;
		l_m.m13 = m10 * m22 * m03 - m20 * m12 * m03 + m20 * m02 * m13 - m00 * m22 * m13 - m10 * m02 * m23 + m00 * m12 * m23;
		l_m.m23 = m20 * m11 * m03 - m10 * m21 * m03 - m20 * m01 * m13 + m00 * m21 * m13 + m10 * m01 * m23 - m00 * m11 * m23;
		l_m.m33 = m10 * m21 * m02 - m20 * m11 * m02 + m20 * m01 * m12 - m00 * m21 * m12 - m10 * m01 * m22 + m00 * m11 * m22;

		l_m = (l_m * (one<T> / this->getDeterminant()));
		return l_m;
	}
	auto getDeterminant() -> T
	{
		T value;

		value =
			m30 * m21 * m12 * m03 - m20 * m31 * m12 * m03 - m30 * m11 * m22 * m03 + m10 * m31 * m22 * m03 +
			m20 * m11 * m32 * m03 - m10 * m21 * m32 * m03 - m30 * m21 * m02 * m13 + m20 * m31 * m02 * m13 +
			m30 * m01 * m22 * m13 - m00 * m31 * m22 * m13 - m20 * m01 * m32 * m13 + m00 * m21 * m32 * m13 +
			m30 * m11 * m02 * m23 - m10 * m31 * m02 * m23 - m30 * m01 * m12 * m23 + m00 * m31 * m12 * m23 +
			m10 * m01 * m32 * m23 - m00 * m11 * m32 * m23 - m20 * m11 * m02 * m33 + m10 * m21 * m02 * m33 +
			m20 * m01 * m12 * m33 - m00 * m21 * m12 * m33 - m10 * m01 * m22 * m33 + m00 * m11 * m22 * m33;

		return value;
	}
};

template<class T>
class TVertex
{
public:
	TVertex() :
		m_pos(TVec4<T>(T(), T(), T(), one<T>)),
		m_texCoord(TVec2<T>(T(), T())),
		m_pad1(TVec2<T>(T(), T())),
		m_normal(TVec4<T>(T(), T(), one<T>, T())),
		m_pad2(TVec4<T>(T(), T(), T(), T())) {};

	TVertex(const TVertex& rhs) :
		m_pos(rhs.m_pos),
		m_pad1(TVec2<T>(T(), T())),
		m_texCoord(rhs.m_texCoord),
		m_normal(rhs.m_normal),
		m_pad2(TVec4<T>(T(), T(), T(), T())) {};

	TVertex(const TVec4<T>& pos, const TVec2<T>& texCoord, const TVec4<T>& normal) :
		m_pos(pos),
		m_pad1(TVec2<T>(T(), T())),
		m_texCoord(texCoord),
		m_normal(normal),
		m_pad2(TVec4<T>(T(), T(), T(), T())) {};

	auto operator=(const TVertex & rhs) -> TVertex<T>&
	{
		m_pos = rhs.m_pos;
		m_pad1 = TVec2<T>(T(), T());
		m_texCoord = rhs.m_texCoord;
		m_normal = rhs.m_normal;
		m_pad2 = TVec4<T>(T(), T(), T(), T());
		return *this;
	}

	~TVertex() {};

	TVec4<T> m_pos; // 4 * sizeof(T)
	TVec2<T> m_texCoord; // 2 * sizeof(T)
	TVec2<T> m_pad1; // 2 * sizeof(T)
	TVec4<T> m_normal; // 4 * sizeof(T)
	TVec4<T> m_pad2; // 4 * sizeof(T)
};

template<class T>
class TRay
{
public:
	TRay() noexcept :
		m_origin(TVec4<T>(T(), T(), T(), one<T>)),
		m_direction(TVec4<T>(T(), T(), T(), T())) {};
	TRay(const TRay<T>& rhs) :
		m_origin(rhs.m_origin),
		m_direction(rhs.m_direction) {};
	auto operator=(const TRay<T> & rhs) -> TRay<T>&
	{
		m_origin = rhs.m_origin;
		m_direction = rhs.m_direction;

		return *this;
	}
	~TRay() {};

	TVec4<T> m_origin; // 4 * sizeof(T)
	TVec4<T> m_direction; // 4 * sizeof(T)
};

template<class T>
class TSphere
{
public:
	TSphere() noexcept :
		m_center(TVec4<T>(T(), T(), T(), one<T>)),
		m_radius(T())
	{};
	TSphere(const TSphere<T>& rhs) :
		m_center(rhs.m_center),
		m_radius(rhs.m_radius) {};
	auto operator=(const TSphere<T> & rhs) -> TSphere<T>&
	{
		m_center = rhs.m_center;
		m_radius = rhs.m_radius;
		return *this;
	}
	~TSphere() {};

	TVec4<T> m_center; // 4 * sizeof(T)
	T m_radius; // 1 * sizeof(T)
};

template<class T>
class TPlane
{
public:
	TPlane() noexcept :
		m_normal(TVec4<T>(T(), T(), T(), one<T>)),
		m_distance(T())
	{};
	TPlane(const TPlane<T>& rhs) :
		m_normal(rhs.m_normal),
		m_distance(rhs.m_distance) {};
	auto operator=(const TPlane<T> & rhs) -> TPlane<T>&
	{
		m_normal = rhs.m_normal;
		m_distance = rhs.m_distance;
		return *this;
	}
	~TPlane() {};

	TVec4<T> m_normal; // 4 * sizeof(T)
	T m_distance; // 1 * sizeof(T)
};

template<class T>
class TAABB
{
public:
	TAABB() noexcept :
		m_center(TVec4<T>(T(), T(), T(), one<T>)),
		m_extend(TVec4<T>(T(), T(), T(), one<T>)),
		m_boundMin(TVec4<T>(T(), T(), T(), one<T>)),
		m_boundMax(TVec4<T>(T(), T(), T(), one<T>)) {};
	TAABB(const TAABB<T>& rhs) :
		m_center(rhs.m_center),
		m_extend(rhs.m_extend),
		m_boundMin(rhs.m_boundMin),
		m_boundMax(rhs.m_boundMax) {};
	auto operator=(const TAABB<T> & rhs) -> TAABB<T>&
	{
		m_center = rhs.m_center;
		m_extend = rhs.m_extend;
		m_boundMin = rhs.m_boundMin;
		m_boundMax = rhs.m_boundMax;
		return *this;
	}
	~TAABB() {};

	TVec4<T> m_center; // 4 * sizeof(T)
	TVec4<T> m_extend; // 4 * sizeof(T)
	TVec4<T> m_boundMin; // 4 * sizeof(T)
	TVec4<T> m_boundMax; // 4 * sizeof(T)
};

template<class T>
class TFrustum
{
public:
	TFrustum() noexcept {};
	TFrustum(const TFrustum<T>& rhs) :
		m_px(rhs.m_px),
		m_nx(rhs.m_nx),
		m_py(rhs.m_py),
		m_ny(rhs.m_ny),
		m_pz(rhs.m_pz),
		m_nz(rhs.m_nz) {};
	auto operator=(const TFrustum<T> & rhs) -> TFrustum<T>&
	{
		m_px = rhs.m_px;
		m_nx = rhs.m_nx;
		m_py = rhs.m_py;
		m_ny = rhs.m_ny;
		m_pz = rhs.m_pz;
		m_nz = rhs.m_nz;
		return *this;
	}
	~TFrustum() {};

	TPlane<T> m_px; // 5 * sizeof(T)
	TPlane<T> m_nx; // 5 * sizeof(T)
	TPlane<T> m_py; // 5 * sizeof(T)
	TPlane<T> m_ny; // 5 * sizeof(T)
	TPlane<T> m_pz; // 5 * sizeof(T)
	TPlane<T> m_nz; // 5 * sizeof(T)
};

template<class T>
class TTransformVector
{
public:
	TTransformVector() noexcept :
		m_pos(TVec4<T>(T(), T(), T(), one<T>)),
		m_rot(TVec4<T>(T(), T(), T(), one<T>)),
		m_scale(TVec4<T>(one<T>, one<T>, one<T>, one<T>)),
		m_pad1(T(), T(), T(), T()) {}
	~TTransformVector() {};

	TVec4<T> m_pos; // 4 * sizeof(T)
	TVec4<T> m_rot; // 4 * sizeof(T)
	TVec4<T> m_scale; // 4 * sizeof(T)
	TVec4<T> m_pad1; // 4 * sizeof(T)
};

template<class T>
class TTransformMatrix
{
public:
	TTransformMatrix() noexcept {}
	~TTransformMatrix() {};

	TMat4<T> m_translationMat; // 16 * sizeof(T)
	TMat4<T> m_rotationMat; // 16 * sizeof(T)
	TMat4<T> m_scaleMat; // 16 * sizeof(T)
	TMat4<T> m_transformationMat; // 16 * sizeof(T)
};

enum direction { FORWARD, BACKWARD, UP, DOWN, RIGHT, LEFT };

namespace InnoMath
{
	template<class T>
	TVec2<T> minVec2 = TVec2<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::min());

	template<class T>
	TVec2<T> maxVec2 = TVec2<T>(std::numeric_limits<T>::max(), std::numeric_limits<T>::max());

	template<class T>
	TVec4<T> minVec4 = TVec4<T>(std::numeric_limits<T>::min(), std::numeric_limits<T>::min(), std::numeric_limits<T>::min(), std::numeric_limits<T>::min());

	template<class T>
	TVec4<T> maxVec4 = TVec4<T>(std::numeric_limits<T>::max(), std::numeric_limits<T>::max(), std::numeric_limits<T>::max(), std::numeric_limits<T>::max());

	template<class T>
	auto isAGreaterThanB(const TVec2<T>& a, const TVec2<T>& b) -> bool
	{
		return a.x > b.x
			&& a.y > b.y;
	}

	template<class T>
	auto isAGreaterThanBVec3(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x > b.x
			&& a.y > b.y
			&& a.z > b.z;
	}

	template<class T>
	auto isAGreaterThanB(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x > b.x
			&& a.y > b.y
			&& a.z > b.z
			&& a.w > b.w;
	}

	template<class T>
	auto isALessThanB(const TVec2<T>& a, const TVec2<T>& b) -> bool
	{
		return a.x < b.x
			&& a.y < b.y;
	}

	template<class T>
	auto isALessThanBVec3(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x < b.x
			&& a.y < b.y
			&& a.z < b.z;
	}

	template<class T>
	auto isALessThanB(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x < b.x
			&& a.y < b.y
			&& a.z < b.z
			&& a.w < b.w;
	}

	template<class T>
	auto lerp(const TVec4<T>& a, const TVec4<T>& b, T alpha) -> TVec4<T>
	{
		return a * alpha + b * (one<T> -alpha);
	}

	template<class T>
	auto slerp(const TVec4<T>& a, const TVec4<T>& b, T alpha) -> TVec4<T>
	{
		T cosOfAngle = a * b;
		// use nlerp for quaternions which are too close
		if (cosOfAngle > one<T> -epsilon4<T>) {
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

			return ((a * -one<T> * s0) + (b * s1)).normalize();
		}
		else
		{
			auto theta_0 = acos(cosOfAngle);
			auto theta = theta_0 * alpha;
			auto sin_theta = std::sin(theta);
			auto sin_theta_0 = std::sin(theta_0);

			auto s0 = sin_theta / sin_theta_0;
			auto s1 = std::cos(theta) - cosOfAngle * sin_theta / sin_theta_0;

			return ((a * s0) + (b * s1)).normalize();
		}
	}

	template<class T>
	auto nlerp(const TVec4<T>& a, const TVec4<T>& b, T alpha) -> TVec4<T>
	{
		return (a * alpha + b * (one<T> -alpha)).normalize();
	}

#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto mul(const TVec4<T> & lhs, const TMat4<T> & rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_TVec4;

		l_TVec4.x = lhs.x * rhs.m00 + lhs.y * rhs.m10 + lhs.z * rhs.m20 + lhs.w * rhs.m30;
		l_TVec4.y = lhs.x * rhs.m01 + lhs.y * rhs.m11 + lhs.z * rhs.m21 + lhs.w * rhs.m31;
		l_TVec4.z = lhs.x * rhs.m02 + lhs.y * rhs.m12 + lhs.z * rhs.m22 + lhs.w * rhs.m32;
		l_TVec4.w = lhs.x * rhs.m03 + lhs.y * rhs.m13 + lhs.z * rhs.m23 + lhs.w * rhs.m33;

		return l_TVec4;
	}
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto mul(const TMat4<T> & lhs, const TVec4<T> & rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_TVec4;

		l_TVec4.x = lhs.m00 * rhs.x + lhs.m01 * rhs.y + lhs.m02 * rhs.z + lhs.m03 * rhs.w;
		l_TVec4.y = lhs.m10 * rhs.x + lhs.m11 * rhs.y + lhs.m12 * rhs.z + lhs.m13 * rhs.w;
		l_TVec4.z = lhs.m20 * rhs.x + lhs.m21 * rhs.y + lhs.m22 * rhs.z + lhs.m23 * rhs.w;
		l_TVec4.w = lhs.m30 * rhs.x + lhs.m31 * rhs.y + lhs.m32 * rhs.z + lhs.m33 * rhs.w;

		return l_TVec4;
	}
#endif

	template<class T>
	auto generateIdentityMatrix() ->TMat4<T>
	{
		TMat4<T> l_m;

		l_m.m00 = one<T>;
		l_m.m11 = one<T>;
		l_m.m22 = one<T>;
		l_m.m33 = one<T>;

		return l_m;
	}

	/*
	 Column-Major memory layout and
	 Row-Major vector4 mathematical convention

	 vector4(a matrix1x4) :
	 | x y z w |

	 matrix4x4
	 [columnIndex][rowIndex]
	 | m00 <-> a00(1.0) m10 <->  a01(0.0) m20 <->  a02(0.0) m30 <->  a03(0.0) |
	 | m01 <-> a10(0.0) m11 <->  a11(1.0) m21 <->  a12(0.0) m31 <->  a13(0.0) |
	 | m02 <-> a20(0.0) m12 <->  a21(0.0) m22 <->  a22(1.0) m32 <->  a23(0.0) |
	 | m03 <-> a30(Tx ) m13 <->  a31(Ty ) m23 <->  a32(Tz ) m33 <->  a33(1.0) |

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

	 matrix4x4
	 [rowIndex][columnIndex]
	 | m00 <-> a00(1.0) m01 <->  a01(0.0) m02 <->  a02(0.0) m03 <->  a03(Tx ) |
	 | m10 <-> a10(0.0) m11 <->  a11(1.0) m12 <->  a12(0.0) m13 <->  a13(Ty ) |
	 | m20 <-> a20(0.0) m21 <->  a21(0.0) m22 <->  a22(1.0) m23 <->  a23(Tz ) |
	 | m30 <-> a30(0.0) m31 <->  a31(0.0) m32 <->  a32(0.0) m33 <->  a33(1.0) |

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
	auto toTranslationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = one<T>;
		l_m.m11 = one<T>;
		l_m.m22 = one<T>;
		l_m.m30 = rhs.x;
		l_m.m31 = rhs.y;
		l_m.m32 = rhs.z;
		l_m.m33 = one<T>;

		return l_m;
	}
	//Row-Major memory layout
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toTranslationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = one<T>;
		l_m.m03 = rhs.x;
		l_m.m11 = one<T>;
		l_m.m13 = rhs.y;
		l_m.m22 = one<T>;
		l_m.m23 = rhs.z;
		l_m.m33 = one<T>;

		return l_m;
	}
#endif

	/*
	 Column-Major memory layout and
	 Row-Major vector4 mathematical convention

	 vector4(a matrix1x4) :
	 | x y z w |

	 matrix4x4
	 [columnIndex][rowIndex]
	 | m00 <-> a00(1 - 2*qy2 - 2*qz2) m10 <->  a01(2*qx*qy + 2*qz*qw) m20 <->  a02(2*qx*qz - 2*qy*qw) m30 <->  a03(0.0) |
	 | m01 <-> a10(2*qx*qy - 2*qz*qw) m11 <->  a11(1 - 2*qx2 - 2*qz2) m21 <->  a12(2*qy*qz + 2*qx*qw) m31 <->  a13(0.0) |
	 | m02 <-> a20(2*qx*qz + 2*qy*qw) m12 <->  a21(2*qy*qz - 2*qx*qw) m22 <->  a22(1 - 2*qx2 - 2*qy2) m32 <->  a23(0.0) |
	 | m03 <-> a30(       0.0       ) m13 <->  a31(       0.0       ) m23 <->  a32(       0.0       ) m33 <->  a33(1.0) |

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

	 matrix4x4
	 [rowIndex][columnIndex]
	 | m00 <-> a00(1 - 2*qy2 - 2*qz2) m01 <->  a01(2*qx*qy - 2*qz*qw) m02 <->  a02(2*qx*qz + 2*qy*qw) m03 <->  a03(0.0) |
	 | m10 <-> a10(2*qx*qy + 2*qz*qw) m11 <->  a11(1 - 2*qx2 - 2*qz2) m12 <->  a12(2*qy*qz - 2*qx*qw) m13 <->  a13(0.0) |
	 | m20 <-> a20(2*qx*qz - 2*qy*qw) m21 <->  a21(2*qy*qz + 2*qx*qw) m22 <->  a22(1 - 2*qx2 - 2*qy2) m23 <->  a23(0.0) |
	 | m30 <-> a30(       0.0       ) m31 <->  a31(       0.0       ) m32 <->  a32(       0.0       ) m33 <->  a33(1.0) |

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

		l_m.m00 = (one<T> -two<T> * rhs.y * rhs.y - two<T> * rhs.z * rhs.z);
		l_m.m01 = (two<T> * rhs.x * y + two<T> * rhs.z * rhs.w);
		l_m.m02 = (two<T> * rhs.x * rhs.z - two<T> * rhs.y * rhs.w);
		l_m.m03 = (T());

		l_m.m10 = (two<T> * rhs.x * rhs.y - two<T> * rhs.z * rhs.w);
		l_m.m11 = (one<T> -two<T> * rhs.x * rhs.x - two<T> * rhs.z * rhs.z);
		l_m.m12 = (two<T> * rhs.y * rhs.z + two<T> * rhs.x * rhs.w);
		l_m.m13 = (T());

		l_m.m20 = (two<T> * rhs.x * rhs.z + two<T> * rhs.y * rhs.w);
		l_m.m21 = (two<T> * rhs.y * rhs.z - two<T> * rhs.x * rhs.w);
		l_m.m22 = (one<T> -two<T> * rhs.x * rhs.x - two<T> * rhs.y * rhs.y);
		l_m.m23 = (T());

		l_m.m30 = (T());
		l_m.m31 = (T());
		l_m.m32 = (T());
		l_m.m33 = (one<T>);

		return l_m;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toRotationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = (one<T> -two<T> *  rhs.y *  rhs.y - two<T> *  rhs.z *  rhs.z);
		l_m.m01 = (two<T> *  rhs.x *  rhs.y - two<T> *  rhs.z *  rhs.w);
		l_m.m02 = (two<T> *  rhs.x *  rhs.z + two<T> *  rhs.y *  rhs.w);
		l_m.m03 = (T());

		l_m.m10 = (two<T> *  rhs.x *  rhs.y + two<T> *  rhs.z *  rhs.w);
		l_m.m11 = (one<T> -two<T> *  rhs.x *  rhs.x - two<T> *  rhs.z *  rhs.z);
		l_m.m12 = (two<T> *  rhs.y *  rhs.z - two<T> *  rhs.x *  rhs.w);
		l_m.m13 = (T());

		l_m.m20 = (two<T> *  rhs.x *  rhs.z - two<T> *  rhs.y *  rhs.w);
		l_m.m21 = (two<T> *  rhs.y *  rhs.z + two<T> *  rhs.x *  rhs.w);
		l_m.m22 = (one<T> -two<T> *  rhs.x *  rhs.x - two<T> *  rhs.y *  rhs.y);
		l_m.m23 = (T());

		l_m.m30 = (T());
		l_m.m31 = (T());
		l_m.m32 = (T());
		l_m.m33 = one<T>;

		return l_m;
	}
#endif

	template<class T>
	auto toScaleMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = rhs.x;
		l_m.m11 = rhs.y;
		l_m.m22 = rhs.z;
		l_m.m33 = rhs.w;

		return l_m;
	}

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toPosVector(const TMat4<T>& rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_result;

		l_result.x = rhs.m30;
		l_result.y = rhs.m31;
		l_result.z = rhs.m32;
		l_result.w = one<T>;

		return l_result;
	}
	//Row-Major memory layout
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toPosVector(const TMat4<T>& rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_result;

		l_result.x = rhs.m03;
		l_result.y = rhs.m13;
		l_result.z = rhs.m23;
		l_result.w = one<T>;

		return l_result;
	}
#endif

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toQuatRotator(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_result;

		T trace = rhs.m00 + rhs.m11 + rhs.m22;
		if (trace > zero<T>)
		{
			T s = half<T> / std::sqrt(trace + one<T>);
			l_result.w = half<T> * half<T> / s;
			l_result.x = (rhs.m12 - rhs.m21) * s;
			l_result.y = (rhs.m20 - rhs.m02) * s;
			l_result.z = (rhs.m01 - rhs.m10) * s;
		}
		else {
			if (rhs.m00 > rhs.m11 && rhs.m00 > rhs.m22)
			{
				T s = two<T> * sqrtf(one<T> +rhs.m00 - rhs.m11 - rhs.m22);
				l_result.w = (rhs.m12 - rhs.m21) / s;
				l_result.x = half<T> * half<T> * s;
				l_result.y = (rhs.m10 + rhs.m01) / s;
				l_result.z = (rhs.m20 + rhs.m02) / s;
			}
			else if (rhs.m11 > rhs.m22)
			{
				T s = two<T> * sqrtf(one<T> +rhs.m11 - rhs.m00 - rhs.m22);
				l_result.w = (rhs.m20 - rhs.m02) / s;
				l_result.x = (rhs.m10 + rhs.m01) / s;
				l_result.y = half<T> * half<T> * s;
				l_result.z = (rhs.m21 + rhs.m12) / s;
			}
			else
			{
				T s = two<T> * sqrtf(one<T> +rhs.m22 - rhs.m00 - rhs.m11);
				l_result.w = (rhs.m01 - rhs.m10) / s;
				l_result.x = (rhs.m20 + rhs.m02) / s;
				l_result.y = (rhs.m21 + rhs.m12) / s;
				l_result.z = half<T> * half<T> * s;
			}
		}

		return l_result;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toQuatRotator(const TMat4<T>& rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_result;

		T trace = rhs.m00 + rhs.m11 + rhs.m22;
		if (trace > zero<T>)
		{
			T s = half<T> / std::sqrt(trace + one<T>);
			l_result.w = half<T> * half<T> / s;
			l_result.x = (rhs.m21 - rhs.m12) * s;
			l_result.y = (rhs.m02 - rhs.m20) * s;
			l_result.z = (rhs.m10 - rhs.m01) * s;
		}
		else {
			if (rhs.m00 > rhs.m11 && rhs.m00 > rhs.m22)
			{
				T s = two<T> * sqrtf(one<T> +rhs.m00 - rhs.m11 - rhs.m22);
				l_result.w = (rhs.m21 - rhs.m12) / s;
				l_result.x = half<T> * half<T> * s;
				l_result.y = (rhs.m01 + rhs.m10) / s;
				l_result.z = (rhs.m02 + rhs.m20) / s;
			}
			else if (rhs.m11 > rhs.m22)
			{
				T s = two<T> * sqrtf(one<T> +rhs.m11 - rhs.m00 - rhs.m22);
				l_result.w = (rhs.m02 - rhs.m20) / s;
				l_result.x = (rhs.m01 + rhs.m10) / s;
				l_result.y = half<T> * half<T> * s;
				l_result.z = (rhs.m12 + rhs.m21) / s;
			}
			else
			{
				T s = two<T> * sqrtf(one<T> +rhs.m22 - rhs.m00 - rhs.m11);
				l_result.w = (rhs.m10 - rhs.m01) / s;
				l_result.x = (rhs.m02 + rhs.m20) / s;
				l_result.y = (rhs.m12 + rhs.m21) / s;
				l_result.z = half<T> * half<T> * s;
			}
		}

		return l_result;
	}
#endif

	template<class T>
	auto toScaleVector(const TMat4<T>& rhs) -> TVec4<T>
	{
		// @TODO: replace with SIMD impl
		TVec4<T> l_result;

		l_result.x = rhs.m00;
		l_result.y = rhs.m11;
		l_result.z = rhs.m22;
		l_result.w = rhs.m33;

		return l_result;
	}

	/*
	Column-Major memory layout and
	Row-Major vector4 mathematical convention

	vector4(a matrix1x4) :
	| x y z w |

	matrix4x4
	[columnIndex][rowIndex]
	| m00 <-> a00(1.0 / (tan(FOV / 2.0) * HWRatio)) m10 <->  a01(         0.0         ) m20 <->  a02(                   0.0                  ) m30 <->  a03(                   0.0                  ) |
	| m01 <-> a10(               0.0              ) m11 <->  a11(1.0 / (tan(FOV / 2.0)) m21 <->  a12(                   0.0                  ) m31 <->  a13(                   0.0                  ) |
	| m02 <-> a20(               0.0              ) m12 <->  a21(         0.0         ) m22 <->  a22(   -(zFar + zNear) / ((zFar - zNear))   ) m32 <->  a23(-(2.0 * zFar * zNear) / ((zFar - zNear))) |
	| m03 <-> a30(               0.0              ) m13 <->  a31(         0.0         ) m23 <->  a32(                  -1.0                  ) m33 <->  a33(                   1.0                  ) |

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

	matrix4x4
	[rowIndex][columnIndex]
	| m00 <-> a00(1.0 / (tan(FOV / 2.0) * HWRatio)) m10 <->  a01(         0.0         ) m20 <->  a02(                   0.0                  ) m30 <->  a03( 0.0) |
	| m01 <-> a01(               0.0              ) m11 <->  a11(1.0 / (tan(FOV / 2.0)) m21 <->  a21(                   0.0                  ) m31 <->  a31( 0.0) |
	| m02 <-> a02(               0.0              ) m12 <->  a12(         0.0         ) m22 <->  a22(   -(zFar + zNear) / ((zFar - zNear))   ) m32 <->  a32(-1.0) |
	| m03 <-> a03(               0.0              ) m13 <->  a13(         0.0         ) m23 <->  a23(-(2.0 * zFar * zNear) / ((zFar - zNear))) m33 <->  a33( 1.0) |

	in

	matrix4x4 * vector4 :
	*/

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto TMat4::generatePerspectiveMatrix(T FOV, T WHRatio, T zNear, T zFar) ->TMat4<T>
	{
		TMat4<T> l_m;

		l_m.m00 = (one<T> / (tan(FOV / two<T>) * WHRatio));
		l_m.m11 = (one<T> / tan(FOV / two<T>));
		l_m.m22 = (-(zFar + zNear) / ((zFar - zNear)));
		l_m.m23 = -one<T>;
		l_m.m32 = (-(two<T> * zFar * zNear) / ((zFar - zNear)));

		return l_m;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto generatePerspectiveMatrix(T FOV, T WHRatio, T zNear, T zFar) ->TMat4<T>
	{
		TMat4<T> l_m;

		l_m.m00 = (one<T> / (tan(FOV / two<T>) * WHRatio));
		l_m.m11 = (one<T> / tan(FOV / two<T>));
		l_m.m22 = (-(zFar + zNear) / ((zFar - zNear)));
		l_m.m23 = (-(two<T> * zFar * zNear) / ((zFar - zNear)));
		l_m.m32 = -one<T>;

		return l_m;
	}
#endif

	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto generateOrthographicMatrix(T left, T right, T bottom, T up, T zNear, T zFar) ->TMat4<T>
	{
		TMat4<T> l_m;

		l_m.m00 = (two<T> / (right - left));
		l_m.m11 = (two<T> / (up - bottom));
		l_m.m22 = (-two<T> / (zFar - zNear));
		l_m.m30 = (-(right + left) / (right - left));
		l_m.m31 = (-(up + bottom) / (up - bottom));
		l_m.m32 = (-(zFar + zNear) / (zFar - zNear));
		l_m.m33 = one<T>;

		return l_m;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto generateOrthographicMatrix(T left, T right, T bottom, T up, T zNear, T zFar) ->TMat4<T>
	{
		TMat4<T> l_m;

		l_m.m00 = (two<T> / (right - left));
		l_m.m03 = (-(right + left) / (right - left));
		l_m.m11 = (two<T> / (up - bottom));
		l_m.m13 = (-(up + bottom) / (up - bottom));
		l_m.m22 = (-two<T> / (zFar - zNear));
		l_m.m23 = (-(zFar + zNear) / (zFar - zNear));
		l_m.m33 = one<T>;

		return l_m;
	}
#endif
	//Column-Major memory layout
#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto lookAt(const TVec4<T> & eyePos, const TVec4<T> & centerPos, const TVec4<T> & upDir) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;
		TVec4<T> l_X;
		TVec4<T> l_Y = upDir;
		TVec4<T> l_Z = TVec4<T>(eyePos.x - centerPos.x, eyePos.y - centerPos.y, eyePos.y - centerPos.y, T()).normalize();

		l_X = l_Y.cross(l_Z);
		l_X = l_X.normalize();
		l_Y = l_Z.cross(l_X);
		l_Y = l_Y.normalize();

		l_m.m00 = l_X.x;
		l_m.m01 = l_Y.x;
		l_m.m02 = l_Z.x;
		l_m.m03 = T();
		l_m.m10 = l_X.y;
		l_m.m11 = l_Y.y;
		l_m.m12 = l_Z.y;
		l_m.m13 = T();
		l_m.m20 = l_X.z;
		l_m.m21 = l_Y.z;
		l_m.m22 = l_Z.z;
		l_m.m23 = T();
		l_m.m30 = -(l_X * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m31 = -(l_Y * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m32 = -(l_Z * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m33 = one<T>;

		return l_m;
	}
	//Row-Major memory layout
#elif defined ( USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto lookAt(const TVec4<T>& eyePos, const TVec4<T>& centerPos, const TVec4<T>& upDir) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;
		TVec4<T> l_X;
		TVec4<T> l_Y = upDir;
		TVec4<T> l_Z = TVec4<T>(eyePos.x - centerPos.x, eyePos.y - centerPos.y, eyePos.y - centerPos.y, T()).normalize();

		l_X = l_Y.cross(l_Z);
		l_X = l_X.normalize();
		l_Y = l_Z.cross(l_X);
		l_Y = l_Y.normalize();

		l_m.m00 = l_X.x;
		l_m.m01 = l_X.y;
		l_m.m02 = l_X.z;
		l_m.m03 = -(l_X * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m10 = l_Y.x;
		l_m.m11 = l_Y.y;
		l_m.m12 = l_Y.z;
		l_m.m13 = -(l_Y * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m20 = l_Z.x;
		l_m.m21 = l_Z.y;
		l_m.m22 = l_Z.z;
		l_m.m23 = -(l_Z * TVec4<T>(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m30 = T();
		l_m.m31 = T();
		l_m.m32 = T();
		l_m.m33 = one<T>;

		return l_m;
	}
#endif

	template<class T>
	bool isPointOnSphere(const TVec4<T> & lhs, const TSphere<T> & rhs)
	{
		auto l_length = (lhs - rhs.m_center).length();
		if (l_length != rhs.m_radius)
		{
			return false;
		}
		return true;
	}

	template<class T>
	bool isPointOnPlane(const TVec4<T> & lhs, const TPlane<T> & rhs)
	{
		auto l_dot = lhs * rhs.m_normal;
		if (l_dot != rhs.m_distance)
		{
			return false;
		}
		return true;
	}

	template<class T>
	bool isPointInAABB(const TVec4<T> & lhs, const TAABB<T> & rhs)
	{
		if (std::abs(rhs.m_center.x - lhs.x) > (rhs.m_extend.x) / two<T>)
		{
			return false;
		}
		if (std::abs(rhs.m_center.y - lhs.y) > (rhs.m_extend.y) / two<T>)
		{
			return false;
		}
		if (std::abs(rhs.m_center.z - lhs.z) > (rhs.m_extend.z) / two<T>)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	template<class T>
	T distanceToPlane(const TVec4<T> & lhs, const TPlane<T> & rhs)
	{
		auto l_dot = lhs * rhs.m_normal;
		return l_dot - rhs.m_distance;
	}

	template<class T>
	bool isPointInFrustum(const TVec4<T> & lhs, const TFrustum<T> & rhs)
	{
		if (distanceToPlane(lhs, rhs.m_px) > zero<T>)
		{
			return false;
		}
		if (distanceToPlane(lhs, rhs.m_nx) > zero<T>)
		{
			return false;
		}
		if (distanceToPlane(lhs, rhs.m_py) > zero<T>)
		{
			return false;
		}
		if (distanceToPlane(lhs, rhs.m_ny) > zero<T>)
		{
			return false;
		}
		if (distanceToPlane(lhs, rhs.m_pz) > zero<T>)
		{
			return false;
		}
		if (distanceToPlane(lhs, rhs.m_nz) > zero<T>)
		{
			return false;
		}
		return true;
	}

	template<class T>
	bool intersectCheck(const TAABB<T> & lhs, const TAABB<T> & rhs)
	{
		if (std::abs(rhs.m_center.x - lhs.m_center.x) > (rhs.m_extend.x + lhs.m_extend.x) / two<T>)
		{
			return false;
		}
		if (std::abs(rhs.m_center.y - lhs.m_center.y) > (rhs.m_extend.y + lhs.m_extend.y) / two<T>)
		{
			return false;
		}
		if (std::abs(rhs.m_center.z - lhs.m_center.z) > (rhs.m_extend.z + lhs.m_extend.z) / two<T>)
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

	template<class T>
	bool intersectCheck(const TFrustum<T> & lhs, const TSphere<T> & rhs)
	{
		if (distanceToPlane(rhs.m_center, lhs.m_px) > rhs.m_radius)
		{
			return false;
		}
		if (distanceToPlane(rhs.m_center, lhs.m_nx) > rhs.m_radius)
		{
			return false;
		}
		if (distanceToPlane(rhs.m_center, lhs.m_py) > rhs.m_radius)
		{
			return false;
		}
		if (distanceToPlane(rhs.m_center, lhs.m_ny) > rhs.m_radius)
		{
			return false;
		}
		if (distanceToPlane(rhs.m_center, lhs.m_pz) > rhs.m_radius)
		{
			return false;
		}
		if (distanceToPlane(rhs.m_center, lhs.m_nz) > rhs.m_radius)
		{
			return false;
		}
		return true;
	}

	template<class T, class U>
	auto precisionConvert(TMat4<T> rhs) ->TMat4<U>
	{
		TMat4<U> l_m;

		l_m.m00 = static_cast<U>(rhs.m00);
		l_m.m01 = static_cast<U>(rhs.m01);
		l_m.m02 = static_cast<U>(rhs.m02);
		l_m.m03 = static_cast<U>(rhs.m03);
		l_m.m10 = static_cast<U>(rhs.m10);
		l_m.m11 = static_cast<U>(rhs.m11);
		l_m.m12 = static_cast<U>(rhs.m12);
		l_m.m13 = static_cast<U>(rhs.m13);
		l_m.m20 = static_cast<U>(rhs.m20);
		l_m.m21 = static_cast<U>(rhs.m21);
		l_m.m22 = static_cast<U>(rhs.m22);
		l_m.m23 = static_cast<U>(rhs.m23);
		l_m.m30 = static_cast<U>(rhs.m30);
		l_m.m31 = static_cast<U>(rhs.m31);
		l_m.m32 = static_cast<U>(rhs.m32);
		l_m.m33 = static_cast<U>(rhs.m33);

		return l_m;
	}

	template<class T>
	auto moveTo(const TVec4<T> & pos, const TVec4<T> & direction, T length)->TVec4<T>
	{
		return pos + direction * length;
	}

	template<class T>
	auto getQuatRotator(const TVec4<T> & axis, T angle)->TVec4<T>
	{
		TVec4<T> normalizedAxis = axis;
		normalizedAxis = normalizedAxis.normalize();
		T sinHalfAngle = std::sin((angle * PI<T> / halfCircumference<T>) / two<T>);
		T cosHalfAngle = std::cos((angle * PI<T> / halfCircumference<T>) / two<T>);

		return TVec4<T>(normalizedAxis.x * sinHalfAngle, normalizedAxis.y * sinHalfAngle, normalizedAxis.z * sinHalfAngle, cosHalfAngle);
	}

	template<class T>
	auto caclRotatedLocalRotator(const TVec4<T> & localRot, const TVec4<T> & axis, T angle)->TVec4<T>
	{
		return getQuatRotator(axis, angle).quatMul(localRot);
	}

	template<class T>
	auto caclRotatedGlobalPositions(const TVec4<T> & localPos, const TVec4<T> & globalPos, const TVec4<T> & axis, T angle)->std::tuple<TVec4<T>, TVec4<T>>
	{
		auto l_rotator = getQuatRotator(axis, angle);

		auto l_globalPos = globalPos;
		l_globalPos.w = T();

		l_globalPos = l_globalPos.rotateDirectionByQuat(l_rotator);
		l_globalPos.w = one<T>;

		auto l_delta = l_globalPos - globalPos;
		auto l_localPos = localPos + l_delta;

		return std::tuple<TVec4<T>, TVec4<T>>(l_localPos, l_globalPos);
	}

	template<class T>
	auto caclGlobalPos(const TMat4<T> & parentTransformationMatrix, const TVec4<T> & localPos) -> TVec4<T>
	{
		//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(localPos, parentTransformationMatrix);
		result = result * (one<T> / result.w);
		return result;
#endif
		//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(parentTransformationMatrix, localPos);
		result = result * (one<T> / result.w);
		return result;
#endif
	}

	template<class T>
	auto caclGlobalRot(const TVec4<T> & parentRot, const TVec4<T> & localRot) -> TVec4<T>
	{
		return parentRot.quatMul(localRot);
	}

	template<class T>
	auto caclGlobalScale(const TVec4<T> parentScale, const TVec4<T> & localScale) -> TVec4<T>
	{
		return parentScale.scale(localScale);
	}

	template<class T>
	auto LocalTransformVectorToGlobal(const TTransformVector<T> & localTransformVector, const TTransformVector<T> & parentTransformVector, const TTransformMatrix<T> & parentTransformMatrix)->TTransformVector<T>
	{
		TTransformVector<T> m;
		m.m_pos = InnoMath::caclGlobalPos(parentTransformMatrix.m_transformationMat, localTransformVector.m_pos);
		m.m_rot = InnoMath::caclGlobalRot(parentTransformVector.m_rot, localTransformVector.m_rot);
		m.m_scale = InnoMath::caclGlobalScale(parentTransformVector.m_scale, localTransformVector.m_scale);
		return m;
	}

	template<class T>
	auto caclTransformationMatrix(TTransformMatrix<T> & transform) -> TMat4<T>
	{
		// @TODO: calculate by hand
		return transform.m_translationMat * transform.m_rotationMat * transform.m_scaleMat;
	}

	template<class T>
	auto TransformVectorToTransformMatrix(const TTransformVector<T> & transformVector)->TTransformMatrix<T>
	{
		TTransformMatrix<T> m;
		m.m_translationMat = InnoMath::toTranslationMatrix(transformVector.m_pos);
		m.m_rotationMat = InnoMath::toRotationMatrix(transformVector.m_rot);
		m.m_scaleMat = InnoMath::toScaleMatrix(transformVector.m_scale);
		m.m_transformationMat = caclTransformationMatrix(m);
		return m;
	}

	template<class T>
	auto caclLookAtMatrix(const TVec4<T> globalPos, const TVec4<T> & localRot) -> TMat4<T>
	{
		return TMat4<T>().lookAt(globalPos, globalPos + getDirection(direction::FORWARD, localRot), getDirection(direction::UP, localRot));
	}

	template<class T>
	auto getInvertTranslationMatrix(const TVec4<T> pos) -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(pos * -one<T>);
	}

	template<class T>
	auto getInvertRotationMatrix(const TVec4<T> rot) -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(rot.quatConjugate());
	}

	template<class T>
	auto getInvertScaleMatrix(const TVec4<T> scale) -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(scale.reciprocal());
	}

	template<class T>
	auto getDirection(direction direction, const TVec4<T>& localRot) -> TVec4<T>
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

		return l_directionTVec4.rotateDirectionByQuat(localRot);
	}

	INNO_FORCEINLINE EntityID createEntityID()
	{
		std::stringstream ss;
		for (unsigned int i = 0; i < 16; i++) {
			auto rc = []() -> unsigned char {
				std::random_device rd;
				std::mt19937 gen(rd());
				std::uniform_int_distribution<> dis(0, 255);
				return static_cast<unsigned char>(dis(gen));
			};
			std::stringstream hexstream;
			hexstream << std::hex << int(rc());
			auto hex = hexstream.str();
			ss << (hex.length() < 2 ? '0' + hex : hex);
		}

		EntityID result = ss.str();
		return result;
	}

	template<class T>
	auto radianToAngle(T radian) -> T
	{
		return radian * fullCircumference<T> / PI<T>;
	}

	template<class T>
	auto angleToRadian(T angle) -> T
	{
		return angle * PI<T> / fullCircumference<T>;
	}

	template<class T>
	auto quatToEulerAngle(const TVec4<T>& rhs) -> TVec4<T>
	{
		// roll (x-axis rotation)
		T sinr_cosp = +two<T> * (rhs.w * rhs.x + rhs.y * rhs.z);
		T cosr_cosp = +one<T> -two<T> * (rhs.x * rhs.x + rhs.y * rhs.y);
		T roll = std::atan2(sinr_cosp, cosr_cosp);

		// pitch (y-axis rotation)
		T sinp = +two<T> * (rhs.w * rhs.y - rhs.z * rhs.x);
		T pitch;
		if (std::fabs(sinp) >= one<T>)
		{
			pitch = std::copysign(PI<T> / two<T>, sinp); // use 90 degrees if out of range
		}
		else
		{
			pitch = std::asin(sinp);
		}

		// yaw (z-axis rotation)
		T siny_cosp = +two<T> * (rhs.w * rhs.z + rhs.x * rhs.y);
		T cosy_cosp = +one<T> -two<T> * (rhs.y * rhs.y + rhs.z * rhs.z);
		T yaw = std::atan2(siny_cosp, cosy_cosp);

		return TVec4<T>(roll, pitch, yaw, zero<T>);
	}

	template<class T>
	auto eulerAngleToQuat(T roll, T pitch, T yaw) -> TVec4<T>
	{
		// Abbreviations for the various angular functions
		T cy = std::cos(yaw * half<T>);
		T sy = std::sin(yaw * half<T>);
		T cp = std::cos(pitch * half<T>);
		T sp = std::sin(pitch * half<T>);
		T cr = std::cos(roll * half<T>);
		T sr = std::sin(roll * half<T>);

		T w = cy * cp * cr + sy * sp * sr;
		T x = cy * cp * sr - sy * sp * cr;
		T y = sy * cp * sr + cy * sp * cr;
		T z = sy * cp * cr - cy * sp * sr;
		return TVec4<T>(x, y, z, w);
	}

	template<class T>
	auto HSVtoRGB(const TVec4<T>& HSV) -> TVec4<T>
	{
		TVec4<T> RGB;
		T h = HSV.x, s = HSV.y, v = HSV.z,
			p, q, t,
			fract;

		(h == two<T> * halfCircumference<T>) ? (h = zero<T>) : (h /= (halfCircumference<T> / (one<T> +two<T>)));
		fract = h - floor(h);

		p = v * (one<T> -s);
		q = v * (one<T> -s * fract);
		t = v * (one<T> -s * (one<T> -fract));

		if (zero<T> <= h && h < one<T>)
			RGB = TVec4<T>(v, t, p, one<T>);
		else if (one<T> <= h && h < two<T>)
			RGB = TVec4<T>(q, v, p, one<T>);
		else if (two<T> <= h && h < one<T> +two<T>)
			RGB = TVec4<T>(p, v, t, one<T>);
		else if (one<T> +two<T> <= h && h < two<T> +two<T>)
			RGB = TVec4<T>(p, q, v, one<T>);
		else if (two<T> +two<T> <= h && h < one<T> +two<T> +two<T>)
			RGB = TVec4<T>(t, p, v, one<T>);
		else if (one<T> +two<T> +two<T> <= h && h < two<T> +two<T> +two<T>)
			RGB = TVec4<T>(v, p, q, one<T>);
		else
			RGB = TVec4<T>(zero<T>, zero<T>, zero<T>, one<T>);

		return RGB;
	}

	template<class T>
	bool isCloseEnough(const TVec4<T>& lhs, const TVec4<T>& rhs)
	{
		if (std::abs(lhs.x - rhs.x) > epsilon4<T>)
		{
			return false;
		}
		if (std::abs(lhs.y - rhs.y) > epsilon4<T>)
		{
			return false;
		}
		if (std::abs(lhs.z - rhs.z) > epsilon4<T>)
		{
			return false;
		}
		if (std::abs(lhs.w - rhs.w) > epsilon4<T>)
		{
			return false;
		}
		else
		{
			return true;
		}
	}

	template<class T>
	auto generateNDC() -> std::vector<TVertex<T>>
	{
		TVertex<T> l_VertexData_1;
		l_VertexData_1.m_pos = TVec4<T>(one<T>, one<T>, one<T>, one<T>);
		l_VertexData_1.m_texCoord = TVec2<T>(one<T>, one<T>);

		TVertex<T> l_VertexData_2;
		l_VertexData_2.m_pos = TVec4<T>(one<T>, -one<T>, one<T>, one<T>);
		l_VertexData_2.m_texCoord = TVec2<T>(one<T>, zero<T>);

		TVertex<T> l_VertexData_3;
		l_VertexData_3.m_pos = TVec4<T>(-one<T>, -one<T>, one<T>, one<T>);
		l_VertexData_3.m_texCoord = TVec2<T>(zero<T>, zero<T>);

		TVertex<T> l_VertexData_4;
		l_VertexData_4.m_pos = TVec4<T>(-one<T>, one<T>, one<T>, one<T>);
		l_VertexData_4.m_texCoord = TVec2<T>(zero<T>, one<T>);

		TVertex<T> l_VertexData_5;
		l_VertexData_5.m_pos = TVec4<T>(one<T>, one<T>, -one<T>, one<T>);
		l_VertexData_5.m_texCoord = TVec2<T>(one<T>, one<T>);

		TVertex<T> l_VertexData_6;
		l_VertexData_6.m_pos = TVec4<T>(one<T>, -one<T>, -one<T>, one<T>);
		l_VertexData_6.m_texCoord = TVec2<T>(one<T>, zero<T>);

		TVertex<T> l_VertexData_7;
		l_VertexData_7.m_pos = TVec4<T>(-one<T>, -one<T>, -one<T>, one<T>);
		l_VertexData_7.m_texCoord = TVec2<T>(zero<T>, zero<T>);

		TVertex<T> l_VertexData_8;
		l_VertexData_8.m_pos = TVec4<T>(-one<T>, one<T>, -one<T>, one<T>);
		l_VertexData_8.m_texCoord = TVec2<T>(zero<T>, one<T>);

		std::vector<TVertex<T>> l_vertices = { l_VertexData_1, l_VertexData_2, l_VertexData_3, l_VertexData_4, l_VertexData_5, l_VertexData_6, l_VertexData_7, l_VertexData_8 };

		for (auto& l_vertexData : l_vertices)
		{
			l_vertexData.m_normal = TVec4<T>(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, zero<T>).normalize();
		}

		return l_vertices;
	}

	template<class T>
	auto makePlane(const TVec4<T>& a, const TVec4<T>& b, const TVec4<T>& c) -> TPlane<T>
	{
		TPlane<T> l_result;

		l_result.m_normal = ((b - a).cross(c - a)).normalize();
		l_result.m_distance = l_result.m_normal * a;

		return l_result;
	};

	template<class T>
	auto makeFrustum(const std::vector<TVertex<T>>& vertices) -> TFrustum<T>
	{
		assert(vertices.size() == 8);

		TFrustum<T> l_result;

		l_result.m_px = makePlane(vertices[2].m_pos, vertices[6].m_pos, vertices[7].m_pos);
		l_result.m_nx = makePlane(vertices[0].m_pos, vertices[4].m_pos, vertices[1].m_pos);
		l_result.m_py = makePlane(vertices[0].m_pos, vertices[3].m_pos, vertices[7].m_pos);
		l_result.m_ny = makePlane(vertices[1].m_pos, vertices[5].m_pos, vertices[6].m_pos);
		l_result.m_pz = makePlane(vertices[0].m_pos, vertices[1].m_pos, vertices[2].m_pos);
		l_result.m_nz = makePlane(vertices[4].m_pos, vertices[7].m_pos, vertices[6].m_pos);

		return l_result;
	};
}

using vec2 = TVec2<float>;
using vec4 = TVec4<float>;
using mat4 = TMat4<float>;
using Vertex = TVertex<float>;
using Ray = TRay<float>;
using AABB = TAABB<float>;
using Sphere = TSphere<float>;
using Plane = TPlane<float>;
using Frustum = TFrustum<float>;
using TransformVector = TTransformVector<float>;
using TransformMatrix = TTransformMatrix<float>;
