#pragma once
#include "../common/stdafx.h"
#include "../common/config.h"

//typedef __m128 vec4;

const static double PI = 3.14159265358979323846264338327950288;

template<class T>
T tolerance2 = T(0.95L);

template<class T>
T tolerance4 = T(0.9995L);

template<class T>
T tolerance8 = T(0.99999995L);

template<class T>
T one = T(1.0L);

template<class T>
T two = T(2.0L);

template<class T>
class vec4;

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
class mat4
{
public:
	mat4()
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
	mat4(const mat4& rhs)
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
	mat4& operator=(const mat4& rhs);
	mat4 operator*(const mat4& rhs);
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	vec4<T> operator*(const vec4<T>& rhs);
#endif
	mat4 operator*(double rhs);
	mat4 transpose()
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
	mat4 inverse()
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
	T getDeterminant()
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
	void initializeToPerspectiveMatrix(T FOV, T WHRatio, T zNear, T zFar);
	void initializeToOrthographicMatrix(T left, T right, T bottom, T up, T zNear, T zFar);
	mat4<T> lookAt(const vec4<T>& eyePos, const vec4<T>& centerPos, const vec4<T>& upDir)
	{
		// @TODO: replace with SIMD impl
		mat4 l_m;
		vec4 l_X;
		vec4 l_Y = upDir;
		vec4 l_Z = vec4(centerPos.x - eyePos.x, centerPos.y - eyePos.y, centerPos.z - eyePos.z, T()).normalize();

		l_X = l_Z.cross(l_Y);
		l_X = l_X.normalize();
		l_Y = l_X.cross(l_Z);
		l_Y = l_Y.normalize();

		l_m.m[0][0] = l_X.x;
		l_m.m[0][1] = l_X.y;
		l_m.m[0][2] = l_X.z;
		l_m.m[0][3] = -(l_X * vec4(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[1][0] = l_Y.x;
		l_m.m[1][1] = l_Y.y;
		l_m.m[1][2] = l_Y.z;
		l_m.m[1][3] = -(l_Y * vec4(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[2][0] = -l_Z.x;
		l_m.m[2][1] = -l_Z.y;
		l_m.m[2][2] = -l_Z.z;
		l_m.m[2][3] = (l_Z * vec4(eyePos.x, eyePos.y, eyePos.z, T()));
		l_m.m[3][0] = T();
		l_m.m[3][1] = T();
		l_m.m[3][2] = T();
		l_m.m[3][3] = one<T>;

		return l_m;
	}
	T m[4][4];
};

template <class T>
// In Homogeneous Coordinates, the w component is a scalar of x, y and z, to represent 3D point vector in 4D, set w to 1.0; to represent 3D direction vector in 4D, set w to 0.0.
// In Quaternion, the w component is sin(theta / 2).
class vec4
{
public:
	vec4() = default;
	vec4(T rhsX, T rhsY, T rhsZ, T rhsW)
	{
		x = rhsX;
		y = rhsY;
		z = rhsZ;
		w = rhsW;
	}
	vec4(const vec4& rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
	}
	vec4<T>& operator=(const vec4<T> & rhs)
	{
		x = rhs.x;
		y = rhs.y;
		z = rhs.z;
		w = rhs.w;
		return *this;
	}

	~vec4()
	{
	}

	vec4<T> operator+(const vec4<T> & rhs) const
	{
		return vec4<T>(x + rhs.x, y + rhs.y, z + rhs.z, w + rhs.w);
	}

	vec4<T> operator+(T rhs) const
	{
		return vec4<T>(x + rhs, y + rhs, z + rhs, w + rhs);
	}

	vec4<T> operator-(const vec4<T> & rhs) const
	{
		return vec4<T>(x - rhs.x, y - rhs.y, z - rhs.z, w - rhs.w);
	}

	vec4<T> operator-(T rhs) const
	{
		return vec4<T>(x - rhs, y - rhs, z - rhs, w - rhs);
	}

	T operator*(const vec4<T> & rhs) const
	{
		return x * rhs.x + y * rhs.y + z * rhs.z + w * rhs.w;
	}

#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	vec4<T> operator*(const mat4<T> & rhs) const
	{
		// @TODO: replace with SIMD impl
		vec4<T> l_vec4;

		l_vec4.x = x * rhs.m[0][0] + y * rhs.m[1][0] + z * rhs.m[2][0] + w * rhs.m[3][0];
		l_vec4.y = x * rhs.m[0][1] + y * rhs.m[1][1] + z * rhs.m[2][1] + w * rhs.m[3][1];
		l_vec4.z = x * rhs.m[0][2] + y * rhs.m[1][2] + z * rhs.m[2][2] + w * rhs.m[3][2];
		l_vec4.w = x * rhs.m[0][3] + y * rhs.m[1][3] + z * rhs.m[2][3] + w * rhs.m[3][3];

		return l_vec4;
	}
#endif

	vec4<T> cross(const vec4<T>& rhs) const
	{
		return vec4<T>(y * rhs.z - z * rhs.y, z * rhs.x - x * rhs.z, x * rhs.y - y * rhs.x, T());
	}

	vec4<T> scale(const vec4<T>& rhs) const
	{
		return vec4<T>(x * rhs.x, y * rhs.y, z * rhs.z, w * rhs.w);
	}

	vec4<T> scale(T rhs) const
	{
		return vec4<T>(x * rhs, y * rhs, z * rhs, w * rhs);
	}

	vec4<T> operator*(T rhs) const
	{
		return vec4<T>(x * rhs, y * rhs, z * rhs, w * rhs);
	}

	vec4<T> operator/(T rhs) const
	{
		return vec4<T>(x / rhs, y / rhs, z / rhs, w / rhs);
	}

	vec4<T> quatMul(const vec4<T>& rhs) const
	{
		vec4<T> l_result = vec4<T>(
			w * rhs.x + x * rhs.w + y * rhs.z - z * rhs.y,
			w * rhs.y - x * rhs.z + y * rhs.w + z * rhs.x,
			w * rhs.z + x * rhs.y - y * rhs.x + z * rhs.w,
			w * rhs.w - x * rhs.x - y * rhs.y - z * rhs.z
			);
		return l_result.normalize();
	}

	vec4<T> quatMul(T rhs) const
	{
		vec4<T> l_result = vec4<T>(
			w * rhs + x * rhs + y * rhs - z * rhs,
			w * rhs - x * rhs + y * rhs + z * rhs,
			w * rhs + x * rhs - y * rhs + z * rhs,
			w * rhs - x * rhs - y * rhs - z * rhs
			);
		return l_result.normalize();
	}

	vec4<T> quatConjugate() const
	{
		return vec4<T>(-x, -y, -z, w);
	}

	vec4<T> reciprocal() const
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

		return vec4<T>(result_x, result_y, result_z, result_w);
	}

	T length() const
	{
		// @TODO: replace with SIMD impl
		return std::sqrt<T>(x * x + y * y + z * z + w * w);
	}

	vec4<T> normalize() const
	{
		// @TODO: replace with SIMD impl
		auto l_length = length();
		return vec4(x / l_length, y / l_length, z / l_length, w / l_length);
	}

	vec4<T> lerp(const vec4<T>& a, const vec4<T>& b, T alpha) const
	{
		return a * alpha + b * (one<T> -alpha);
	}

	vec4<T> slerp(const vec4<T>& a, const vec4<T>& b, T alpha) const
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

	vec4 nlerp(const vec4<T>& a, const vec4<T>& b, T alpha) const
	{
		return (a * alpha + b * (one<T> -alpha)).normalize();
	}

	bool operator!=(const vec4<T> & rhs)
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

	bool operator==(const vec4<T> & rhs)
	{
		// @TODO: replace with SIMD impl
		return !(*this != rhs);
	}

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
	mat4<T> toTranslationMatrix()
	{
		// @TODO: replace with SIMD impl
		mat4 l_m;

		l_m.m[0][0] = 1;
		l_m.m[0][3] = x;
		l_m.m[1][1] = 1;
		l_m.m[1][3] = y;
		l_m.m[2][2] = 1;
		l_m.m[2][3] = z;
		l_m.m[3][3] = 1;

		return l_m;
	}
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
	mat4<T> toRotationMatrix()
	{
		// @TODO: replace with SIMD impl
		mat4 l_m;

		l_m.m[0][0] = (one<T> -two<T> * y * y - two<T> * z * z);
		l_m.m[0][1] = (two<T> * x * y - two<T> * z * w);
		l_m.m[0][2] = (two<T> * x * z + two<T> * y * w);
		l_m.m[0][3] = (T());

		l_m.m[1][0] = (two<T> * x * y + two<T> * z * w);
		l_m.m[1][1] = (one<T> -two<T> * x * x - two<T> * z * z);
		l_m.m[1][2] = (two<T> * y * z - two<T> * x * w);
		l_m.m[1][3] = (T());

		l_m.m[2][0] = (two<T> * x * z - two<T> * y * w);
		l_m.m[2][1] = (two<T> * y * z + two<T> * x * w);
		l_m.m[2][2] = (one<T> -two<T> * x * x - two<T> * y * y);
		l_m.m[2][3] = (0);

		l_m.m[3][0] = (T());
		l_m.m[3][1] = (T());
		l_m.m[3][2] = (T());
		l_m.m[3][3] = one<T>;

		return l_m;
	}
	mat4<T> toScaleMatrix()
	{
		// @TODO: replace with SIMD impl
		mat4 l_m;
		l_m.m[0][0] = x;
		l_m.m[1][1] = y;
		l_m.m[2][2] = z;
		l_m.m[3][3] = w;
		return l_m;
	}

	T x;
	T y;
	T z;
	T w;
};

template<class T>
class vec2
{
public:
	vec2();
	vec2(T rhsX, T rhsY)
	{
		x = rhsX;
		y = rhsY;
	}
	vec2(const vec2& rhs)
	{
		x = rhs.x;
		y = rhs.y;
	}
	vec2& operator=(const vec2& rhs);

	~vec2()
	{
	}

	vec2 operator+(const vec2& rhs);
	vec2 operator+(T rhs);
	vec2 operator-(const vec2& rhs);
	vec2 operator-(T rhs);
	T operator*(const vec2& rhs);
	vec2 scale(const vec2& rhs)
	{
		return vec2(x * rhs.x, y * rhs.y);
	}
	vec2 scale(T rhs);
	vec2 operator*(T rhs);
	vec2 operator/(T rhs);
	T length()
	{
		return sqrt(x * x + y * y);
	}
	vec2 normalize()
	{
		return vec2(x / length(), y / length());
	}

	T x;
	T y;
};

class Vertex
{
public:
	Vertex() :
		m_pos(vec4<double>(0.0, 0.0, 0.0, 1.0)),
		m_texCoord(vec2<double>(0.0, 0.0)),
		m_normal(vec4<double>(0.0, 0.0, 1.0, 0.0)) {};
	Vertex(const Vertex& rhs) :
		m_pos(rhs.m_pos),
		m_texCoord(rhs.m_texCoord),
		m_normal(rhs.m_normal) {};
	Vertex(const vec4<double>& pos, const vec2<double>& texCoord, const vec4<double>& normal) :
		m_pos(pos),
		m_texCoord(texCoord),
		m_normal(normal) {};
	Vertex& operator=(const Vertex& rhs);
	~Vertex() {};

	vec4<double> m_pos;
	vec2<double> m_texCoord;
	vec4<double> m_normal;
};

class Ray
{
public:
	Ray():
		m_origin(vec4<double>(0.0, 0.0, 0.0, 1.0)),
		m_direction(vec4<double>(0.0, 0.0, 0.0, 0.0)) {};
	Ray(const Ray& rhs) :
		m_origin(rhs.m_origin),
		m_direction(rhs.m_direction) {};
	Ray& operator=(const Ray& rhs);
	~Ray() {};

	vec4<double> m_origin;
	vec4<double> m_direction;
};

class AABB
{
public:
	AABB() :
		m_center(vec4<double>(0.0, 0.0, 0.0, 1.0)),
		m_sphereRadius(0.0),
		m_boundMin(vec4<double>(0.0, 0.0, 0.0, 1.0)),
		m_boundMax(vec4<double>(0.0, 0.0, 0.0, 1.0)) {};;
	AABB(const AABB& rhs) :
		m_center(rhs.m_center),
		m_sphereRadius(rhs.m_sphereRadius),
		m_boundMin(rhs.m_boundMin),
		m_boundMax(rhs.m_boundMax),
		m_vertices(rhs.m_vertices),
		m_indices(rhs.m_indices) {};
	AABB& operator=(const AABB& rhs);
	~AABB() {};

	vec4<double> m_center;
	double m_sphereRadius;
	vec4<double> m_boundMin;
	vec4<double> m_boundMax;

	std::vector<Vertex> m_vertices;
	std::vector<unsigned int> m_indices;

	bool intersectCheck(const AABB& rhs);
	bool intersectCheck(const Ray& rhs);
};

enum direction { FORWARD, BACKWARD, UP, DOWN, RIGHT, LEFT };

class Transform
{
public:
	Transform() : 
		m_parentTransform(nullptr),
		m_pos(vec4<double>(0.0, 0.0, 0.0, 1.0)),
		m_rot(vec4<double>(0.0, 0.0, 0.0, 1.0)),
		m_scale(vec4<double>(1.0, 1.0, 1.0, 1.0)),
		m_previousPos((m_pos + (1.0)) / 2.0),
		m_previousRot(m_rot.quatMul(vec4<double>(1.0, 1.0, 1.0, 0.0))),
		m_previousScale(m_scale + (1.0)) {};
	~Transform() {};

	bool hasChanged();
	void update();
	void rotateInLocal(const vec4<double> & axis, double angle);

	vec4<double>& getPos();
	vec4<double>& getRot();
	vec4<double>& getScale();

	void setLocalPos(const vec4<double>& pos);
	void setLocalRot(const vec4<double>& rot);
	void setLocalScale(const vec4<double>& scale);

	mat4<double> caclLocalTranslationMatrix();
	mat4<double> caclLocalRotMatrix();
	mat4<double> caclLocalScaleMatrix();
	mat4<double> caclLocalTransformationMatrix();
	mat4<double> caclPreviousLocalTranslationMatrix();
	mat4<double> caclPreviousLocalRotMatrix();
	mat4<double> caclPreviousLocalScaleMatrix();
	mat4<double> caclPreviousLocalTransformationMatrix();

	vec4<double> caclGlobalPos();
	vec4<double> caclGlobalRot();
	vec4<double> caclGlobalScale();
	vec4<double> caclPreviousGlobalPos();
	vec4<double> caclPreviousGlobalRot();
	vec4<double> caclPreviousGlobalScale();

	mat4<double> caclGlobalTranslationMatrix();
	mat4<double> caclGlobalRotMatrix();
	mat4<double> caclGlobalScaleMatrix();
	mat4<double> caclGlobalTransformationMatrix();
	mat4<double> caclPreviousGlobalTranslationMatrix();
	mat4<double> caclPreviousGlobalRotMatrix();
	mat4<double> caclPreviousGlobalScaleMatrix();
	mat4<double> caclPreviousGlobalTransformationMatrix();

	mat4<double> caclLookAtMatrix();
	mat4<double> caclPreviousLookAtMatrix();

	mat4<double> getInvertLocalTranslationMatrix();
	mat4<double> getInvertLocalRotMatrix();
	mat4<double> getInvertLocalScaleMatrix();
	mat4<double> getInvertGlobalTranslationMatrix();
	mat4<double> getInvertGlobalRotMatrix();
	mat4<double> getInvertGlobalScaleMatrix();
	mat4<double> getPreviousInvertLocalTranslationMatrix();
	mat4<double> getPreviousInvertLocalRotMatrix();
	mat4<double> getPreviousInvertLocalScaleMatrix();
	mat4<double> getPreviousInvertGlobalTranslationMatrix();
	mat4<double> getPreviousInvertGlobalRotMatrix();
	mat4<double> getPreviousInvertGlobalScaleMatrix();
	vec4<double> getDirection(direction direction) const;
	vec4<double> getPreviousDirection(direction direction) const;
	Transform* m_parentTransform;

private:
	vec4<double> m_pos;
	vec4<double> m_rot;
	vec4<double> m_scale;

	vec4<double> m_previousPos;
	vec4<double> m_previousRot;
	vec4<double> m_previousScale;
};
