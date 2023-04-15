#pragma once
#include "../Common/STL14.h"
#include "../Common/Config.h"
#include "../Common/InnoType.h"

// typedef __m128 TVec4;

#undef max
#undef min

namespace Inno
{
	namespace InnoMath
	{
		template <class T>
		const static T PI = T(3.14159265358979323846264338327950288L);

		template <class T>
		const static T E = T(2.71828182845904523536028747135266249L);

		template <class T, size_t precision>
		T epsilon = T(0.00000001L);

		template <class T>
		T epsilon<T, 1> = T(0.1L);

		template <class T>
		T epsilon<T, 2> = T(0.01L);

		template <class T>
		T epsilon<T, 3> = T(0.001L);

		template <class T>
		T epsilon<T, 4> = T(0.0001L);

		template <class T>
		T epsilon<T, 5> = T(0.00001L);

		template <class T>
		T epsilon<T, 6> = T(0.000001L);

		template <class T>
		T epsilon<T, 7> = T(0.0000001L);

		template <class T>
		T epsilon<T, 8> = T(0.00000001L);

		template <class T>
		T zero = T(0.0L);

		template <class T>
		T half = T(0.5L);

		template <class T>
		T one = T(1.0L);

		template <class T>
		T two = T(2.0L);

		template <class T>
		T halfCircumference = T(180.0L);

		template <class T>
		T fullCircumference = T(360.0L);

		template <class T>
		class TVec2
		{
		public:
			T x;
			T y;

			TVec2(): x(), y()
			{
			}

			TVec2(T rhsX, T rhsY): x(rhsX), y(rhsY)
			{
			}

			TVec2(const TVec2& rhs)
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

			auto operator[](size_t i) -> T&
			{
				return (&x)[i];
			}

			~TVec2()
			{
			}

			auto operator+(const TVec2<T>& rhs) const -> TVec2<T>
			{
				return TVec2<T>(x + rhs.x, y + rhs.y);
			}

			auto operator+(T rhs) const -> TVec2<T>
			{
				return TVec2<T>(x + rhs, y + rhs);
			}

			auto operator-(const TVec2<T>& rhs) const -> TVec2<T>
			{
				return TVec2<T>(x - rhs.x, y - rhs.y);
			}

			auto operator-(T rhs) const -> TVec2<T>
			{
				return TVec2<T>(x - rhs, y - rhs);
			}

			auto operator*(const TVec2<T>& rhs) const -> T
			{
				return x * rhs.x + y * rhs.y;
			}

			auto scale(const TVec2<T>& rhs) const -> TVec2<T>
			{
				return TVec2<T>(x * rhs.x, y * rhs.y);
			}

			auto scale(T rhs) const -> TVec2<T>
			{
				return TVec2<T>(x * rhs, y * rhs);
			}
			auto operator*(T rhs) const -> TVec2<T>
			{
				return TVec2<T>(x * rhs, y * rhs);
			}

			auto operator/(T rhs) const -> TVec2<T>
			{
				assert(rhs);
				return TVec2<T>(x / rhs, y / rhs);
			}

			auto length() const -> T
			{
				return std::sqrt(x * x + y * y);
			}

			auto normalize() const -> TVec2<T>
			{
				auto l_length = length();
				if (l_length != T())
				{
					return TVec2<T>(x / l_length, y / l_length);
				}

				return TVec2<T>();
			}
		};

		template <class T>
		class TVec4;

		template <class T>
		class TVec3
		{
			friend class TVec4<T>;

		public:
			T x;
			T y;
			T z;

			TVec3(): x(), y(), z()
			{
			}

			TVec3(T rhsX, T rhsY, T rhsZ): x(rhsX), y(rhsY), z(rhsZ)
			{
			}

			TVec3(const TVec3& rhs)
			{
				x = rhs.x;
				y = rhs.y;
				z = rhs.z;
			}

			auto operator=(const TVec3<T>& rhs) -> TVec3<T>&
			{
				x = rhs.x;
				y = rhs.y;
				z = rhs.z;

				return *this;
			}

			auto operator[](size_t i) -> T&
			{
				return (&x)[i];
			}

			~TVec3()
			{
			}

			auto operator+(const TVec3<T>& rhs) const -> TVec3<T>
			{
				return TVec3<T>(x + rhs.x, y + rhs.y, z + rhs.z);
			}

			auto operator+(T rhs) const -> TVec3<T>
			{
				return TVec3<T>(x + rhs, y + rhs, z + rhs);
			}

			auto operator-(const TVec3<T>& rhs) const -> TVec3<T>
			{
				return TVec3<T>(x - rhs.x, y - rhs.y, z - rhs.z);
			}

			auto operator-(T rhs) const -> TVec3<T>
			{
				return TVec3<T>(x - rhs, y - rhs, z - rhs);
			}

			auto operator*(const TVec3<T>& rhs) const -> T
			{
				return x * rhs.x + y * rhs.y + z * rhs.z;
			}

			auto cross(const TVec3<T>& rhs) const -> TVec3<T>
			{
				return TVec3<T>(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x);
			}

			auto scale(const TVec3<T>& rhs) const -> TVec3<T>
			{
				return TVec3<T>(x * rhs.x, y * rhs.y, z * rhs.z);
			}

			auto operator*(T rhs) const -> TVec3<T>
			{
				return TVec3<T>(x * rhs, y * rhs, z * rhs);
			}

			auto operator/(T rhs) const -> TVec3<T>
			{
				assert(rhs);
				return TVec3<T>(x / rhs, y / rhs, z / rhs);
			}

			auto reciprocal() const -> TVec3<T>
			{
				T result_x = T();
				T result_y = T();
				T result_z = T();

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

				return TVec3<T>(result_x, result_y, result_z);
			}

			auto length() const -> T
			{
				// @TODO: replace with SIMD impl
				return std::sqrt(x * x + y * y + z * z);
			}

			auto normalize() const -> TVec3<T>
			{
				// @TODO: replace with SIMD impl
				auto l_length = length();
				if (l_length != T())
				{
					return TVec3(x / l_length, y / l_length, z / l_length);
				}

				return TVec3<T>();
			}

			bool operator!=(const TVec3<T>& rhs) const
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

			bool operator==(const TVec3<T>& rhs) const
			{
				// @TODO: replace with SIMD impl
				return !(*this != rhs);
			}
		};

		// In Homogeneous Coordinates, the w component is a scalar of x, y and z. In case to represent 3D point vector in 4D, set w to 1.0; to represent 3D direction vector in 4D, set w to 0.0.
		// In Quaternion, the w component is sin(theta / 2).
		template <class T>
		class TVec4
		{
		public:
			T x;
			T y;
			T z;
			T w;

			TVec4(): x(), y(), z(), w()
			{
			}

			TVec4(T rhsX, T rhsY, T rhsZ, T rhsW): x(rhsX), y(rhsY), z(rhsZ), w(rhsW)
			{
			}

			TVec4(const TVec4& rhs)
			{
				x = rhs.x;
				y = rhs.y;
				z = rhs.z;
				w = rhs.w;
			}

			auto operator=(const TVec4<T>& rhs) -> TVec4<T>&
			{
				x = rhs.x;
				y = rhs.y;
				z = rhs.z;
				w = rhs.w;

				return *this;
			}

			TVec3<T> xyz() const { return TVec3<T>(x, y, z); }

			TVec4(const TVec3<T>& rhs, T rhs_w = zero<T>)
			{
				x = rhs.x;
				y = rhs.y;
				z = rhs.z;
				w = rhs_w;
			}

			auto operator=(const TVec3<T>& rhs) -> TVec4<T>&
			{
				x = rhs.x;
				y = rhs.y;
				z = rhs.z;
				w = zero<T>;
				return *this;
			}

			auto operator[](size_t i) -> T&
			{
				return (&x)[i];
			}

			~TVec4()
			{
			}

			auto operator+(const TVec4<T>& rhs) const -> TVec4<T>
			{
				return TVec4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
			}

			auto operator+(T rhs) const -> TVec4<T>
			{
				return TVec4<T>(x + rhs, y + rhs, z + rhs, w + rhs);
			}

			auto operator-(const TVec4<T>& rhs) const -> TVec4<T>
			{
				return TVec4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
			}

			auto operator-(T rhs) const -> TVec4<T>
			{
				return TVec4<T>(x - rhs, y - rhs, z - rhs, w - rhs);
			}

			auto operator*(const TVec4<T>& rhs) const -> T
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
				assert(rhs);
				return TVec4<T>(x / rhs, y / rhs, z / rhs, w / rhs);
			}

			auto quatMul(const TVec4<T>& rhs) const -> TVec4<T>
			{
				TVec4<T> l_result = TVec4<T>(
					w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
					w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
					w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
					w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z);
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

			auto length() const -> T
			{
				// @TODO: replace with SIMD impl
				return std::sqrt(x * x + y * y + z * z + w * w);
			}

			auto normalize() const -> TVec4<T>
			{
				// @TODO: replace with SIMD impl
				auto l_length = length();
				if (l_length != T())
				{
					return TVec4(x / l_length, y / l_length, z / l_length, w / l_length);
				}

				return TVec4<T>();
			}

			bool operator!=(const TVec4<T>& rhs) const
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

			bool operator==(const TVec4<T>& rhs) const
			{
				// @TODO: replace with SIMD impl
				return !(*this != rhs);
			}

			auto rotateDirectionByQuat(const TVec4<T>& rhs) const -> TVec4<T>
			{
				// V' = QVQ^-1, for unit quaternion, the conjugated quaternion is as same as the inverse quaternion

				// naive version
				// get Q * V by hand
				// TVec4 l_hiddenRotatedQuat;
				// l_hiddenRotatedQuat.w = -m_rot.x * l_directionTVec4.x - m_rot.y * l_directionTVec4.y - m_rot.z * l_directionTVec4.z;
				// l_hiddenRotatedQuat.x = m_rot.w * l_directionTVec4.x + m_rot.y * l_directionTVec4.z - m_rot.z * l_directionTVec4.y;
				// l_hiddenRotatedQuat.y = m_rot.w * l_directionTVec4.y + m_rot.z * l_directionTVec4.x - m_rot.x * l_directionTVec4.z;
				// l_hiddenRotatedQuat.z = m_rot.w * l_directionTVec4.z + m_rot.x * l_directionTVec4.y - m_rot.y * l_directionTVec4.x;

				// get conjugated quaternion
				// TVec4 l_conjugatedQuat;
				// l_conjugatedQuat = conjugate(m_rot);

				// then QV * Q^-1
				// TVec4 l_directionQuat;
				// l_directionQuat = l_hiddenRotatedQuat * l_conjugatedQuat;
				// l_directionTVec4.x = l_directionQuat.x;
				// l_directionTVec4.y = l_directionQuat.y;
				// l_directionTVec4.z = l_directionQuat.z;

				// traditional version, change direction vector to quaternion representation

				// TVec4 l_directionQuat = TVec4(0.0, l_directionTVec4);
				// l_directionQuat = m_rot * l_directionQuat * conjugate(m_rot);
				// l_directionTVec4.x = l_directionQuat.x;
				// l_directionTVec4.y = l_directionQuat.y;
				// l_directionTVec4.z = l_directionQuat.z;

				// optimized version ([Kavan et al. ] Lemma 4)
				// V' = V + 2 * Qv x (Qv x V + Qs * V)
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

		template <class T>
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

#if defined(USE_COLUMN_MAJOR_MEMORY_LAYOUT)
			auto TMat4::operator*(const TMat4<T>& rhs) const -> TMat4<T>
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
#elif defined(USE_ROW_MAJOR_MEMORY_LAYOUT)
			auto operator*(const TMat4<T>& rhs) const -> TMat4<T>
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

			auto operator*(const T rhs) const -> TMat4<T>
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

			auto transpose() const -> TMat4<T>
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

			auto inverse() const -> TMat4<T>
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

			auto getDeterminant() const -> T
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

#pragma pack(push, 1)
		template <class T>
		class TVertex
		{
		public:
			TVertex() noexcept: m_pos(),
				m_normal(T(), T(), one<T>),
				m_tangent(one<T>, T(), T()),
				m_texCoord(),
				m_pad1(),
				m_pad2() {
				std::fill(m_pad1, m_pad1 + 4, T());
			};

			TVertex(const TVertex& rhs): m_pos(rhs.m_pos),
				m_normal(rhs.m_normal),
				m_tangent(rhs.m_tangent),
				m_texCoord(rhs.m_texCoord),
				m_pad2(rhs.m_pad2) {
				std::memcpy(m_pad1, rhs.m_pad1, sizeof(T) * 4);
			};

			TVertex(const TVec3<T>& pos, const TVec3<T>& normal, const TVec3<T>& tangent, const TVec2<T>& texCoord): m_pos(pos),
				m_normal(normal),
				m_tangent(tangent),
				m_texCoord(texCoord) {};

			auto operator=(const TVertex& rhs) -> TVertex<T>&
			{
				m_pos = rhs.m_pos;
				m_normal = rhs.m_normal;
				m_tangent = rhs.m_tangent;
				m_texCoord = rhs.m_texCoord;
				std::memcpy(m_pad1, rhs.m_pad1, sizeof(T) * 4);
				m_pad2 = rhs.m_pad2;
				return *this;
			}

			~TVertex() {};

			TVec3<T> m_pos;		 // 3 * sizeof(T)
			TVec3<T> m_normal;	 // 3 * sizeof(T)
			TVec3<T> m_tangent;	 // 3 * sizeof(T)
			TVec2<T> m_texCoord; // 2 * sizeof(T)
			T m_pad1[4];		 // 4 * sizeof(T)
			T m_pad2;			 // 1 * sizeof(T)
		};
#pragma pack(pop)

		template <class T>
		class TRay
		{
		public:
			TRay() noexcept: m_origin(TVec4<T>(T(), T(), T(), one<T>)),
				m_direction(TVec4<T>(T(), T(), T(), T())) {};

			TRay(const TRay<T>& rhs): m_origin(rhs.m_origin),
				m_direction(rhs.m_direction) {};

			auto operator=(const TRay<T>& rhs) -> TRay<T>&
			{
				m_origin = rhs.m_origin;
				m_direction = rhs.m_direction;

				return *this;
			}

			~TRay() {};

			TVec4<T> m_origin;	  // 4 * sizeof(T)
			TVec4<T> m_direction; // 4 * sizeof(T)
		};

#pragma pack(push, 1)
		template <class T>
		class TSphere
		{
		public:
			TSphere() noexcept: m_center(TVec3<T>(T(), T(), T())),
				m_radius(T()) {};

			TSphere(const TSphere<T>& rhs): m_center(rhs.m_center),
				m_radius(rhs.m_radius) {};

			auto operator=(const TSphere<T>& rhs) -> TSphere<T>&
			{
				m_center = rhs.m_center;
				m_radius = rhs.m_radius;
				return *this;
			}

			~TSphere() {};

			TVec3<T> m_center; // 3 * sizeof(T)
			T m_radius;		   // 1 * sizeof(T)
		};

		template <class T>
		class TPlane
		{
		public:
			TPlane() noexcept: m_normal(TVec3<T>(T(), T(), one<T>)),
				m_distance(T()) {};

			TPlane(const TPlane<T>& rhs): m_normal(rhs.m_normal),
				m_distance(rhs.m_distance) {};

			auto operator=(const TPlane<T>& rhs) -> TPlane<T>&
			{
				m_normal = rhs.m_normal;
				m_distance = rhs.m_distance;
				return *this;
			}

			~TPlane() {};

			TVec3<T> m_normal; // 3 * sizeof(T)
			T m_distance;	   // 1 * sizeof(T)
		};
#pragma pack(pop)

		template <class T>
		class TAABB
		{
		public:
			TAABB()
				noexcept: m_center(TVec4<T>(T(), T(), T(), one<T>)),
				m_extend(TVec4<T>(T(), T(), T(), one<T>)),
				m_boundMin(TVec4<T>(T(), T(), T(), one<T>)),
				m_boundMax(TVec4<T>(T(), T(), T(), one<T>)) {};

			TAABB(const TAABB<T>& rhs): m_center(rhs.m_center),
				m_extend(rhs.m_extend),
				m_boundMin(rhs.m_boundMin),
				m_boundMax(rhs.m_boundMax) {};

			auto operator=(const TAABB<T>& rhs) -> TAABB<T>&
			{
				m_center = rhs.m_center;
				m_extend = rhs.m_extend;
				m_boundMin = rhs.m_boundMin;
				m_boundMax = rhs.m_boundMax;
				return *this;
			}

			~TAABB() {};

			TVec4<T> m_center;	 // 4 * sizeof(T)
			TVec4<T> m_extend;	 // 4 * sizeof(T)
			TVec4<T> m_boundMin; // 4 * sizeof(T)
			TVec4<T> m_boundMax; // 4 * sizeof(T)
		};

		template <class T>
		class TFrustum
		{
		public:
			TFrustum() noexcept {};

			TFrustum(const TFrustum<T>& rhs): m_px(rhs.m_px),
				m_nx(rhs.m_nx),
				m_py(rhs.m_py),
				m_ny(rhs.m_ny),
				m_pz(rhs.m_pz),
				m_nz(rhs.m_nz) {};

			auto operator=(const TFrustum<T>& rhs) -> TFrustum<T>&
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

			TPlane<T> m_px; // 4 * sizeof(T)
			TPlane<T> m_nx; // 4 * sizeof(T)
			TPlane<T> m_py; // 4 * sizeof(T)
			TPlane<T> m_ny; // 4 * sizeof(T)
			TPlane<T> m_pz; // 4 * sizeof(T)
			TPlane<T> m_nz; // 4 * sizeof(T)
		};

		template <class T>
		class TTransformVector
		{
		public:
			TTransformVector() noexcept: m_pos(TVec4<T>(T(), T(), T(), one<T>)),
				m_rot(TVec4<T>(T(), T(), T(), one<T>)),
				m_scale(TVec4<T>(one<T>, one<T>, one<T>, one<T>)),
				m_pad1(T(), T(), T(), T()) {}

			~TTransformVector() {};

			TVec4<T> m_pos;	  // 4 * sizeof(T)
			TVec4<T> m_rot;	  // 4 * sizeof(T)
			TVec4<T> m_scale; // 4 * sizeof(T)
			TVec4<T> m_pad1;  // 4 * sizeof(T)
		};

		template <class T>
		class TTransformMatrix
		{
		public:
			TTransformMatrix() noexcept {}
			~TTransformMatrix() {};

			TMat4<T> m_translationMat;	  // 16 * sizeof(T)
			TMat4<T> m_rotationMat;		  // 16 * sizeof(T)
			TMat4<T> m_scaleMat;		  // 16 * sizeof(T)
			TMat4<T> m_transformationMat; // 16 * sizeof(T)
		};

		template <class T>
		class TSH9
		{
		public:
			TSH9()
				noexcept {}
			~TSH9() {};

			auto operator+=(const TSH9<T>& rhs) -> TSH9<T>
			{
				L00 = L00 + rhs.L00;
				L11 = L11 + rhs.L11;
				L10 = L10 + rhs.L10;
				L1_1 = L1_1 + rhs.L1_1;
				L21 = L21 + rhs.L21;
				L2_1 = L2_1 + rhs.L2_1;
				L2_2 = L2_2 + rhs.L2_2;
				L20 = L20 + rhs.L20;
				L22 = L22 + rhs.L22;

				return *this;
			}

			auto operator/=(T rhs) -> TSH9<T>
			{
				L00 = L00 / rhs;
				L11 = L11 / rhs;
				L10 = L10 / rhs;
				L1_1 = L1_1 / rhs;
				L21 = L21 / rhs;
				L2_1 = L2_1 / rhs;
				L2_2 = L2_2 / rhs;
				L20 = L20 / rhs;
				L22 = L22 / rhs;

				return *this;
			}

			TVec4<T> L00;
			TVec4<T> L11;
			TVec4<T> L10;
			TVec4<T> L1_1;
			TVec4<T> L21;
			TVec4<T> L2_1;
			TVec4<T> L2_2;
			TVec4<T> L20;
			TVec4<T> L22;
		};

		enum class Direction
		{
			Forward,
			Backward,
			Up,
			Down,
			Right,
			Left
		};

		using Vec2 = TVec2<float>;
		using Vec3 = TVec3<float>;
		using Vec4 = TVec4<float>;
		using Mat4 = TMat4<float>;
		using Vertex = TVertex<float>;
		using Ray = TRay<float>;
		using AABB = TAABB<float>;
		using Sphere = TSphere<float>;
		using Plane = TPlane<float>;
		using Frustum = TFrustum<float>;
		using TransformVector = TTransformVector<float>;
		using TransformMatrix = TTransformMatrix<float>;
		using SH9 = TSH9<float>;
	}
}
