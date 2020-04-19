#pragma once
#include "InnoMath.h"

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
	auto isAGreaterEqualThanB(const TVec2<T>& a, const TVec2<T>& b) -> bool
	{
		return a.x >= b.x
			&& a.y >= b.y;
	}

	template<class T>
	auto isAGreaterEqualThanBVec3(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x >= b.x
			&& a.y >= b.y
			&& a.z >= b.z;
	}

	template<class T>
	auto isAGreaterEqualThanB(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x >= b.x
			&& a.y >= b.y
			&& a.z >= b.z
			&& a.w >= b.w;
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
	auto isALessEqualThanB(const TVec2<T>& a, const TVec2<T>& b) -> bool
	{
		return a.x <= b.x
			&& a.y <= b.y;
	}

	template<class T>
	auto isALessEqualThanBVec3(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x <= b.x
			&& a.y <= b.y
			&& a.z <= b.z;
	}

	template<class T>
	auto isALessEqualThanB(const TVec4<T>& a, const TVec4<T>& b) -> bool
	{
		return a.x <= b.x
			&& a.y <= b.y
			&& a.z <= b.z
			&& a.w <= b.w;
	}

	template<class T>
	auto clamp(T x, T min, T max)->T
	{
		if (x < min) { return min; }
		if (x > max) { return max; }
		return x;
	}

	template<class T>
	auto clamp(const TVec2<T>& x, const TVec2<T>& min, const TVec2<T>& max)->TVec2<T>
	{
		if (isALessThanB(x, min)) { return min; }
		if (isAGreaterThanB(x, max)) { return max; }
		return x;
	}

	template<class T>
	auto clampVec3(const TVec4<T>& x, const TVec4<T>& min, const TVec4<T>& max)->TVec4<T>
	{
		if (isALessThanBVec3(x, min)) { return min; }
		if (isAGreaterThanBVec3(x, max)) { return max; }
		return x;
	}

	template<class T>
	auto clamp(const TVec4<T>& x, const TVec4<T>& min, const TVec4<T>& max)->TVec4<T>
	{
		if (isALessThanB(x, min)) { return min; }
		if (isAGreaterThanB(x, max)) { return max; }
		return x;
	}

	template<class T>
	auto elementWiseMax(const TVec4<T>& A, const TVec4<T>& B)->TVec4<T>
	{
		TVec4<T> l_result;

		if (A.x >= B.x)
		{
			l_result.x = A.x;
		}
		else
		{
			l_result.x = B.x;
		}
		if (A.y >= B.y)
		{
			l_result.y = A.y;
		}
		else
		{
			l_result.y = B.y;
		}
		if (A.z >= B.z)
		{
			l_result.z = A.z;
		}
		else
		{
			l_result.z = B.z;
		}
		if (A.w >= B.w)
		{
			l_result.w = A.w;
		}
		else
		{
			l_result.w = B.w;
		}

		return l_result;
	}

	template<class T>
	auto elementWiseMin(const TVec4<T>& A, const TVec4<T>& B)->TVec4<T>
	{
		TVec4<T> l_result;

		if (A.x <= B.x)
		{
			l_result.x = A.x;
		}
		else
		{
			l_result.x = B.x;
		}
		if (A.y <= B.y)
		{
			l_result.y = A.y;
		}
		else
		{
			l_result.y = B.y;
		}
		if (A.z <= B.z)
		{
			l_result.z = A.z;
		}
		else
		{
			l_result.z = B.z;
		}
		if (A.w <= B.w)
		{
			l_result.w = A.w;
		}
		else
		{
			l_result.w = B.w;
		}

		return l_result;
	}

	template<class T>
	auto lerp(const TVec2<T>& a, const TVec2<T>& b, T alpha) -> TVec2<T>
	{
		return a * alpha + b * (one<T> -alpha);
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
		if (cosOfAngle > one<T> -epsilon<T, 4>)
		{
			return (a * alpha + b * (one<T>-alpha)).normalize();
		}
		// for shorter path
		if (cosOfAngle < T())
		{
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
	auto mul(const TVec4<T>& lhs, const TMat4<T>& rhs) -> TVec4<T>
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
	auto mul(const TMat4<T>& lhs, const TVec4<T>& rhs) -> TVec4<T>
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
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto toRotationMatrix(const TVec4<T>& rhs) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;

		l_m.m00 = (one<T> -two<T> * rhs.y * rhs.y - two<T> * rhs.z * rhs.z);
		l_m.m01 = (two<T> * rhs.x * rhs.y - two<T> * rhs.z * rhs.w);
		l_m.m02 = (two<T> * rhs.x * rhs.z + two<T> * rhs.y * rhs.w);
		l_m.m03 = (T());

		l_m.m10 = (two<T> * rhs.x * rhs.y + two<T> * rhs.z * rhs.w);
		l_m.m11 = (one<T> -two<T> * rhs.x * rhs.x - two<T> * rhs.z * rhs.z);
		l_m.m12 = (two<T> * rhs.y * rhs.z - two<T> * rhs.x * rhs.w);
		l_m.m13 = (T());

		l_m.m20 = (two<T> * rhs.x * rhs.z - two<T> * rhs.y * rhs.w);
		l_m.m21 = (two<T> * rhs.y * rhs.z + two<T> * rhs.x * rhs.w);
		l_m.m22 = (one<T> -two<T> * rhs.x * rhs.x - two<T> * rhs.y * rhs.y);
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
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
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

#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	/*
	Assume symmetric view frustum

	Column-Major matrix memory layout and
	Row-Major vector4 mathematical convention

	vector4(a matrix1x4) :
	| x y z w |

	matrix4x4
	[columnIndex][rowIndex]
	| m00 <-> a00(1.0/tan(FOVX/2.0)) m10 <->  a01(             0.0             ) m20 <->  a02(                0.0               ) m30 <->  a03(                   0.0                  ) |
	| m01 <-> a10(       0.0       ) m11 <->  a11(1.0/(tan(FOVX/2.0) / WHRatio)) m21 <->  a12(                0.0               ) m31 <->  a13(                   0.0                  ) |
	| m02 <-> a20(       0.0       ) m12 <->  a21(             0.0             ) m22 <->  a22(-(zFar + zNear) / ((zFar - zNear))) m32 <->  a23(-(2.0 * zFar * zNear) / ((zFar - zNear))) |
	| m03 <-> a30(       0.0       ) m13 <->  a31(              0.0            ) m23 <->  a32(               -1.0               ) m33 <->  a33(                   1.0                  ) |
	*/
	template<class T>
	auto TMat4::generatePerspectiveMatrix(T FOV, T WHRatio, T zNear, T zFar) ->TMat4<T>
	{
		TMat4<T> l_m;

		l_m.m00 = one<T> / tan(FOVX / two<T>);
		l_m.m11 = one<T> / (tan(FOVX / two<T>) / WHRatio);
		l_m.m22 = (-(zFar + zNear) / ((zFar - zNear)));
		l_m.m23 = -one<T>;
		l_m.m32 = (-(two<T> * zFar * zNear) / ((zFar - zNear)));

		return l_m;
	}
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	/*
	Assume symmetric view frustum

	Row-Major matrix memory layout and
	Column-Major vector4 mathematical convention

	vector4(a matrix4x1) :
	| x |
	| y |
	| z |
	| w |

	[rowIndex][columnIndex]
	| m00 <-> a00(1.0/tan(FOVX/2.0)) m10 <-> a01(              0.0            ) m20 <-> a02(                   0.0                  ) m30 <-> a03( 0.0) |
	| m01 <-> a01(       0.0       ) m11 <-> a11(1.0/(tan(FOVX/2.0) / WHRatio)) m21 <-> a21(                   0.0                  ) m31 <-> a31( 0.0) |
	| m02 <-> a02(       0.0       ) m12 <-> a12(              0.0            ) m22 <-> a22(   -(zFar + zNear) / ((zFar - zNear))   ) m32 <-> a32(-1.0) |
	| m03 <-> a03(       0.0       ) m13 <-> a13(              0.0            ) m23 <-> a23(-(2.0 * zFar * zNear) / ((zFar - zNear))) m33 <-> a33( 1.0) |
	*/
	template<class T>
	auto generatePerspectiveMatrix(T FOVX, T WHRatio, T zNear, T zFar) ->TMat4<T>
	{
		TMat4<T> l_m;

		l_m.m00 = one<T> / tan(FOVX / two<T>);
		l_m.m11 = one<T> / (tan(FOVX / two<T>) / WHRatio);
		l_m.m22 = (-(zFar + zNear) / ((zFar - zNear)));
		l_m.m23 = (-(two<T> * zFar * zNear) / ((zFar - zNear)));
		l_m.m32 = -one<T>;

		return l_m;
	}
#endif

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
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
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

#if defined (USE_COLUMN_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto lookAt(const TVec4<T>& eyePos, const TVec4<T>& centerPos, const TVec4<T>& upDir) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;
		TVec4<T> l_X;
		TVec4<T> l_Y;
		TVec4<T> l_Z = (eyePos - centerPos).normalize();

		l_X = upDir.cross(l_Z);
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
		l_m.m30 = eyePos.x;
		l_m.m31 = eyePos.y;
		l_m.m32 = eyePos.z;
		l_m.m33 = one<T>;

		return l_m;
	}
#elif defined (USE_ROW_MAJOR_MEMORY_LAYOUT)
	template<class T>
	auto lookAt(const TVec4<T>& eyePos, const TVec4<T>& centerPos, const TVec4<T>& upDir) -> TMat4<T>
	{
		// @TODO: replace with SIMD impl
		TMat4<T> l_m;
		TVec4<T> l_X;
		TVec4<T> l_Y;
		TVec4<T> l_Z = (eyePos - centerPos).normalize();

		l_X = upDir.cross(l_Z);
		l_X = l_X.normalize();
		l_Y = l_Z.cross(l_X);
		l_Y = l_Y.normalize();

		l_m.m00 = l_X.x;
		l_m.m01 = l_X.y;
		l_m.m02 = l_X.z;
		l_m.m03 = eyePos.x;
		l_m.m10 = l_Y.x;
		l_m.m11 = l_Y.y;
		l_m.m12 = l_Y.z;
		l_m.m13 = eyePos.y;
		l_m.m20 = l_Z.x;
		l_m.m21 = l_Z.y;
		l_m.m22 = l_Z.z;
		l_m.m23 = eyePos.z;
		l_m.m30 = T();
		l_m.m31 = T();
		l_m.m32 = T();
		l_m.m33 = one<T>;

		return l_m;
	}
#endif

	template<class T>
	auto worldToViewSpace(const TVec4<T>& rhs, const TMat4<T>& cameraT, const TMat4<T>& cameraR) ->TVec4<T>
	{
		auto l_result = rhs;
		auto l_m = cameraT * cameraR;
		l_m = l_m.inverse();
#if defined USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_result = mul(l_result, l_m);
#elif defined USE_ROW_MAJOR_MEMORY_LAYOUT
		l_result = mul(l_m, l_result);
#endif
		return l_result;
	}

	template<class T>
	auto viewToWorldSpace(const TVec4<T>& rhs, const TMat4<T>& cameraT, const TMat4<T>& cameraR) ->TVec4<T>
	{
		auto l_result = rhs;
		auto l_m = cameraT * cameraR;
#if defined USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_result = mul(l_result, l_m);
#elif defined USE_ROW_MAJOR_MEMORY_LAYOUT
		l_result = mul(l_m, l_result);
#endif
		return l_result;
	}

	template<class T>
	auto viewToClipSpace(const TVec4<T>& rhs, const TMat4<T>& cameraP) ->TVec4<T>
	{
		auto l_result = rhs;
#if defined USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_result = InnoMath::mul(l_result, cameraP);
#elif defined USE_ROW_MAJOR_MEMORY_LAYOUT
		l_result = InnoMath::mul(cameraP, l_result);
#endif
		// perspective division
		l_result = l_result * (one<T> / l_result.w);

		return l_result;
	}

	template<class T>
	auto clipToViewSpace(const TVec4<T>& rhs, const TMat4<T>& cameraP) ->TVec4<T>
	{
		auto l_result = rhs;
#if defined USE_COLUMN_MAJOR_MEMORY_LAYOUT
		l_result = InnoMath::mul(l_result, cameraP.inverse());
#elif defined USE_ROW_MAJOR_MEMORY_LAYOUT
		l_result = InnoMath::mul(cameraP.inverse(), l_result);
#endif
		// perspective division
		l_result = l_result * (one<T> / l_result.w);

		return l_result;
	}

	template<class T>
	T distanceToSphere(const TVec4<T>& lhs, const TSphere<T>& rhs)
	{
		auto l_dir = lhs - rhs.m_center;
		return l_dir.length() - rhs.m_radius;
	}

	template<class T>
	T distanceToPlane(const TVec4<T>& lhs, const TPlane<T>& rhs)
	{
		auto l_dot = lhs * rhs.m_normal;
		return l_dot - rhs.m_distance;
	}

	template<class T>
	T closestPoint(const TVec4<T>& lhs, const TSphere<T>& rhs)
	{
		auto l_dir = lhs - rhs.m_center;
		l_dir = l_dir.normalize();
		return l_dir * rhs.m_radius + rhs.m_center;
	}

	template<class T>
	T closestPoint(const TVec4<T>& lhs, const TPlane<T>& rhs)
	{
		auto l_dot = lhs * rhs.m_normal;
		auto l_distance = l_dot - rhs.m_distance;
		return lhs - rhs.m_normal * l_distance;
	}

	template<class T>
	bool isPointOnSphere(const TVec4<T>& lhs, const TSphere<T>& rhs)
	{
		auto l_distance = (lhs - rhs.m_center).length();
		if (std::abs(l_distance - rhs.m_radius) > epsilon<T, 4>)
		{
			return false;
		}
		return true;
	}

	template<class T>
	bool isPointOnPlane(const TVec4<T>& lhs, const TPlane<T>& rhs)
	{
		auto l_distance = lhs * rhs.m_normal;
		if (std::abs(l_distance - rhs.m_distance) > epsilon<T, 4>)
		{
			return false;
		}
		return true;
	}

	template<class T>
	bool isPointInAABB(const TVec4<T>& lhs, const TAABB<T>& rhs)
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
	bool isPointInFrustum(const TVec4<T>& lhs, const TFrustum<T>& rhs)
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
	bool intersectCheck(const TAABB<T>& lhs, const TAABB<T>& rhs)
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
	bool intersectCheck(const TAABB<T>& lhs, const TRay<T>& rhs)
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
	bool intersectCheck(const TFrustum<T>& lhs, const TSphere<T>& rhs)
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

	template<class T>
	bool intersectCheck(const TFrustum<T>& lhs, const TAABB<T>& rhs)
	{
		auto l_isCenterInside = isPointInFrustum(rhs.m_center, lhs);
		if (l_isCenterInside)
		{
			return true;
		}

		auto l_isMaxInside = isPointInFrustum(rhs.m_boundMax, lhs);
		if (l_isMaxInside)
		{
			return true;
		}

		auto l_isMinInside = isPointInFrustum(rhs.m_boundMin, lhs);
		if (l_isMinInside)
		{
			return true;
		}

		return false;
	}

	template<class T, class U>
	auto precisionConvert(const TVec2<T>& rhs) ->TVec2<U>
	{
		TVec2<U> l_result;

		l_result.x = static_cast<U>(rhs.x);
		l_result.y = static_cast<U>(rhs.y);

		return l_result;
	}

	template<class T, class U>
	auto precisionConvert(const TVec4<T>& rhs) ->TVec4<U>
	{
		TVec4<U> l_result;

		l_result.x = static_cast<U>(rhs.x);
		l_result.y = static_cast<U>(rhs.y);
		l_result.z = static_cast<U>(rhs.z);
		l_result.w = static_cast<U>(rhs.y);

		return l_result;
	}

	template<class T, class U>
	auto precisionConvert(const TMat4<T>& rhs) ->TMat4<U>
	{
		TMat4<U> l_result;

		l_result.m00 = static_cast<U>(rhs.m00);
		l_result.m01 = static_cast<U>(rhs.m01);
		l_result.m02 = static_cast<U>(rhs.m02);
		l_result.m03 = static_cast<U>(rhs.m03);
		l_result.m10 = static_cast<U>(rhs.m10);
		l_result.m11 = static_cast<U>(rhs.m11);
		l_result.m12 = static_cast<U>(rhs.m12);
		l_result.m13 = static_cast<U>(rhs.m13);
		l_result.m20 = static_cast<U>(rhs.m20);
		l_result.m21 = static_cast<U>(rhs.m21);
		l_result.m22 = static_cast<U>(rhs.m22);
		l_result.m23 = static_cast<U>(rhs.m23);
		l_result.m30 = static_cast<U>(rhs.m30);
		l_result.m31 = static_cast<U>(rhs.m31);
		l_result.m32 = static_cast<U>(rhs.m32);
		l_result.m33 = static_cast<U>(rhs.m33);

		return l_result;
	}

	template<class T>
	auto moveTo(const TVec4<T>& pos, const TVec4<T>& direction, T length)->TVec4<T>
	{
		return pos + direction * length;
	}

	template<class T>
	auto getQuatRotator(const TVec4<T>& axis, T angle)->TVec4<T>
	{
		TVec4<T> normalizedAxis = axis;
		normalizedAxis = normalizedAxis.normalize();
		T sinHalfAngle = std::sin((angle * PI<T> / halfCircumference<T>) / two<T>);
		T cosHalfAngle = std::cos((angle * PI<T> / halfCircumference<T>) / two<T>);

		return TVec4<T>(normalizedAxis.x * sinHalfAngle, normalizedAxis.y * sinHalfAngle, normalizedAxis.z * sinHalfAngle, cosHalfAngle);
	}

	template<class T>
	auto calcRotatedLocalRotator(const TVec4<T>& localRot, const TVec4<T>& axis, T angle)->TVec4<T>
	{
		return getQuatRotator(axis, angle).quatMul(localRot);
	}

	template<class T>
	auto calcRotatedGlobalPositions(const TVec4<T>& localPos, const TVec4<T>& globalPos, const TVec4<T>& axis, T angle)->std::tuple<TVec4<T>, TVec4<T>>
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
	auto calcGlobalPos(const TMat4<T>& parentTransformationMatrix, const TVec4<T>& localPos) -> TVec4<T>
	{
#if defined USE_COLUMN_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(localPos, parentTransformationMatrix);
		result = result * (one<T> / result.w);
		return result;
#elif defined USE_ROW_MAJOR_MEMORY_LAYOUT
		auto result = TVec4<T>();
		result = InnoMath::mul(parentTransformationMatrix, localPos);
		result = result * (one<T> / result.w);
		return result;
#endif
	}

	template<class T>
	auto calcGlobalRot(const TVec4<T>& parentRot, const TVec4<T>& localRot) -> TVec4<T>
	{
		return parentRot.quatMul(localRot);
	}

	template<class T>
	auto calcGlobalScale(const TVec4<T>& parentScale, const TVec4<T>& localScale) -> TVec4<T>
	{
		return parentScale.scale(localScale);
	}

	template<class T>
	auto LocalTransformVectorToGlobal(const TTransformVector<T>& localTransformVector, const TTransformVector<T>& parentTransformVector, const TTransformMatrix<T>& parentTransformMatrix)->TTransformVector<T>
	{
		TTransformVector<T> m;
		m.m_pos = InnoMath::calcGlobalPos(parentTransformMatrix.m_transformationMat, localTransformVector.m_pos);
		m.m_rot = InnoMath::calcGlobalRot(parentTransformVector.m_rot, localTransformVector.m_rot);
		m.m_scale = InnoMath::calcGlobalScale(parentTransformVector.m_scale, localTransformVector.m_scale);
		return m;
	}

	template<class T>
	auto calcTransformationMatrix(const TTransformMatrix<T>& transform) -> TMat4<T>
	{
		// @TODO: calculate by hand
		return transform.m_translationMat * transform.m_rotationMat * transform.m_scaleMat;
	}

	template<class T>
	auto TransformVectorToTransformMatrix(const TTransformVector<T>& transformVector)->TTransformMatrix<T>
	{
		TTransformMatrix<T> m;
		m.m_translationMat = InnoMath::toTranslationMatrix(transformVector.m_pos);
		m.m_rotationMat = InnoMath::toRotationMatrix(transformVector.m_rot);
		m.m_scaleMat = InnoMath::toScaleMatrix(transformVector.m_scale);
		m.m_transformationMat = calcTransformationMatrix(m);
		return m;
	}

	template<class T>
	auto calcLookAtMatrix(const TVec4<T>& globalPos, const TVec4<T>& localRot) -> TMat4<T>
	{
		return TMat4<T>().lookAt(globalPos, globalPos + getDirection(Direction::Forward, localRot), getDirection(Direction::Up, localRot));
	}

	template<class T>
	auto getInvertTranslationMatrix(const TVec4<T>& pos) -> TMat4<T>
	{
		return InnoMath::toTranslationMatrix(pos * -one<T>);
	}

	template<class T>
	auto getInvertRotationMatrix(const TVec4<T>& rot) -> TMat4<T>
	{
		return InnoMath::toRotationMatrix(rot.quatConjugate());
	}

	template<class T>
	auto getInvertScaleMatrix(const TVec4<T>& scale) -> TMat4<T>
	{
		return InnoMath::toScaleMatrix(scale.reciprocal());
	}

	template<class T>
	auto getDirection(Direction direction, const TVec4<T>& localRot) -> TVec4<T>
	{
		TVec4<T> l_directionTVec4;

		switch (direction)
		{
		case Direction::Forward: l_directionTVec4 = TVec4<T>(0.0f, 0.0f, 1.0f, 0.0f); break;
		case Direction::Backward:l_directionTVec4 = TVec4<T>(0.0f, 0.0f, -1.0f, 0.0f); break;
		case Direction::Up:l_directionTVec4 = TVec4<T>(0.0f, 1.0f, 0.0f, 0.0f); break;
		case Direction::Down:l_directionTVec4 = TVec4<T>(0.0f, -1.0f, 0.0f, 0.0f); break;
		case Direction::Right:l_directionTVec4 = TVec4<T>(1.0f, 0.0f, 0.0f, 0.0f); break;
		case Direction::Left:l_directionTVec4 = TVec4<T>(-1.0f, 0.0f, 0.0f, 0.0f); break;
		}

		return l_directionTVec4.rotateDirectionByQuat(localRot);
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

	template<class T, size_t precision>
	bool isCloseEnough(const TVec4<T>& lhs, const TVec4<T>& rhs)
	{
		if (std::abs(lhs.x - rhs.x) > epsilon<T, precision>)
		{
			return false;
		}
		if (std::abs(lhs.y - rhs.y) > epsilon<T, precision>)
		{
			return false;
		}
		if (std::abs(lhs.z - rhs.z) > epsilon<T, precision>)
		{
			return false;
		}
		if (std::abs(lhs.w - rhs.w) > epsilon<T, precision>)
		{
			return false;
		}
		else
		{
			return true;
		}
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
	inline auto generateNDC(TVertex<T>* vertices)
	{
		vertices[0].m_pos = TVec4<T>(one<T>, one<T>, one<T>, one<T>);
		vertices[0].m_texCoord = TVec2<T>(one<T>, one<T>);

		vertices[1].m_pos = TVec4<T>(one<T>, -one<T>, one<T>, one<T>);
		vertices[1].m_texCoord = TVec2<T>(one<T>, zero<T>);

		vertices[2].m_pos = TVec4<T>(-one<T>, -one<T>, one<T>, one<T>);
		vertices[2].m_texCoord = TVec2<T>(zero<T>, zero<T>);

		vertices[3].m_pos = TVec4<T>(-one<T>, one<T>, one<T>, one<T>);
		vertices[3].m_texCoord = TVec2<T>(zero<T>, one<T>);

		vertices[4].m_pos = TVec4<T>(one<T>, one<T>, -one<T>, one<T>);
		vertices[4].m_texCoord = TVec2<T>(one<T>, one<T>);

		vertices[5].m_pos = TVec4<T>(one<T>, -one<T>, -one<T>, one<T>);
		vertices[5].m_texCoord = TVec2<T>(one<T>, zero<T>);

		vertices[6].m_pos = TVec4<T>(-one<T>, -one<T>, -one<T>, one<T>);
		vertices[6].m_texCoord = TVec2<T>(zero<T>, zero<T>);

		vertices[7].m_pos = TVec4<T>(-one<T>, one<T>, -one<T>, one<T>);
		vertices[7].m_texCoord = TVec2<T>(zero<T>, one<T>);

		for (size_t i = 0; i < 8; i++)
		{
			vertices[i].m_normal = TVec4<T>(vertices[i].m_pos.x, vertices[i].m_pos.y, vertices[i].m_pos.z, zero<T>).normalize();
		}
	}

	template<class T>
	inline auto generateAABB(TVec4<T> boundMax, TVec4<T> boundMin) -> TAABB<T>
	{
		TAABB<T> l_result;

		l_result.m_boundMin = boundMin;
		l_result.m_boundMax = boundMax;

		l_result.m_center = (boundMax + boundMin) * half<T>;
		l_result.m_extend = boundMax - boundMin;
		l_result.m_extend.w = one<T>;

		return l_result;
	}

	template<class T>
	inline auto generateAABB(TVertex<T>* vertices, size_t size) -> TAABB<T>
	{
		TAABB<T> l_result;

		T maxX = vertices[0].m_pos.x;
		T maxY = vertices[0].m_pos.y;
		T maxZ = vertices[0].m_pos.z;
		T minX = vertices[0].m_pos.x;
		T minY = vertices[0].m_pos.y;
		T minZ = vertices[0].m_pos.z;

		for (size_t i = 0; i < size; i++)
		{
			if (vertices[i].m_pos.x >= maxX)
			{
				maxX = vertices[i].m_pos.x;
			}
			if (vertices[i].m_pos.y >= maxY)
			{
				maxY = vertices[i].m_pos.y;
			}
			if (vertices[i].m_pos.z >= maxZ)
			{
				maxZ = vertices[i].m_pos.z;
			}
			if (vertices[i].m_pos.x <= minX)
			{
				minX = vertices[i].m_pos.x;
			}
			if (vertices[i].m_pos.y <= minY)
			{
				minY = vertices[i].m_pos.y;
			}
			if (vertices[i].m_pos.z <= minZ)
			{
				minZ = vertices[i].m_pos.z;
			}
		}

		return generateAABB(TVec4(maxX, maxY, maxZ, one<T>), TVec4(minX, minY, minZ, one<T>));

		return l_result;
	}

	template<class T>
	inline auto generateBoundSphere(const TAABB<T>& rhs) -> TSphere<T>
	{
		TSphere<T> l_result;
		l_result.m_center = rhs.m_center;
		l_result.m_radius = (rhs.m_boundMax - rhs.m_center).length();

		return l_result;
	}

	template<class T>
	inline auto makeFrustum(const TVertex<T>* vertices) -> TFrustum<T>
	{
		TFrustum<T> l_result;

		l_result.m_px = makePlane(vertices[2].m_pos, vertices[6].m_pos, vertices[7].m_pos);
		l_result.m_nx = makePlane(vertices[0].m_pos, vertices[4].m_pos, vertices[1].m_pos);
		l_result.m_py = makePlane(vertices[0].m_pos, vertices[3].m_pos, vertices[7].m_pos);
		l_result.m_ny = makePlane(vertices[1].m_pos, vertices[5].m_pos, vertices[6].m_pos);
		l_result.m_pz = makePlane(vertices[0].m_pos, vertices[1].m_pos, vertices[2].m_pos);
		l_result.m_nz = makePlane(vertices[4].m_pos, vertices[7].m_pos, vertices[6].m_pos);

		return l_result;
	}

	template<class T>
	inline auto transformAABBSpace(const TAABB<T>& rhs, TMat4<T> Tm) -> TAABB<T>
	{
		auto l_vertices = generateAABBVertices(rhs.m_boundMax, rhs.m_boundMin);

		for (auto& i : l_vertices)
		{
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
			i.m_pos = InnoMath::mul(i.m_pos, Tm);
#endif
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
			i.m_pos = InnoMath::mul(Tm, i.m_pos);
#endif
		}

		auto l_boundMin = maxVec4<T>;
		auto l_boundMax = minVec4<T>;

		for (auto& i : l_vertices)
		{
			l_boundMin = elementWiseMin(i.m_pos, l_boundMin);
			l_boundMax = elementWiseMax(i.m_pos, l_boundMax);
		}

		l_boundMin.w = one<T>;
		l_boundMax.w = one<T>;

		auto l_result = generateAABB(l_boundMax, l_boundMin);

		return l_result;
	};

	inline std::vector<Vertex> worldToViewSpace(const std::vector<Vertex>& rhs, Mat4 t, Mat4 r)
	{
		auto l_result = rhs;

		for (auto& l_vertexData : l_result)
		{
			auto l_mulPos = InnoMath::worldToViewSpace(l_vertexData.m_pos, t, r);
			l_vertexData.m_pos = l_mulPos;
		}

		for (auto& l_vertexData : l_result)
		{
			l_vertexData.m_normal = Vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
		}

		return l_result;
	}

	inline std::vector<Vertex> viewToWorldSpace(const std::vector<Vertex>& rhs, Mat4 t, Mat4 r)
	{
		auto l_result = rhs;

		for (auto& l_vertexData : l_result)
		{
			auto l_mulPos = InnoMath::viewToWorldSpace(l_vertexData.m_pos, t, r);
			l_vertexData.m_pos = l_mulPos;
		}

		for (auto& l_vertexData : l_result)
		{
			l_vertexData.m_normal = Vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
		}

		return l_result;
	}

	inline std::vector<Vertex> generateFrustumVerticesVS(Mat4 p)
	{
		std::vector<Vertex> rhs(8);

		InnoMath::generateNDC<float>(&rhs[0]);

		for (auto& i : rhs)
		{
			i.m_pos = InnoMath::clipToViewSpace(i.m_pos, p);
		}

		// near clip plane first
		// @TODO: reverse only along Z axis, not simple mirrored version
		std::reverse(rhs.begin(), rhs.end());

		std::vector<Vertex> l_vertices(8);

		for (uint32_t i = 0; i < 8; i++)
		{
			l_vertices[i] = rhs[i];
		}

		return l_vertices;
	}

	inline std::vector<Vertex> generateFrustumVerticesWS(Mat4 p, Mat4 r, Mat4 t)
	{
		auto rhs = generateFrustumVerticesVS(p);

		for (auto& i : rhs)
		{
			i.m_pos = InnoMath::viewToWorldSpace(i.m_pos, t, r);
		}

		for (auto& i : rhs)
		{
			i.m_normal = Vec4(i.m_pos.x, i.m_pos.y, i.m_pos.z, 0.0f).normalize();
		}

		return rhs;
	}

	inline std::vector<Vertex> generateAABBVertices(Vec4 boundMax, Vec4 boundMin)
	{
		std::vector<Vertex> l_vertices(8);

		l_vertices[0].m_pos = (Vec4(boundMax.x, boundMax.y, boundMax.z, 1.0f));
		l_vertices[0].m_texCoord = Vec2(1.0f, 1.0f);

		l_vertices[1].m_pos = (Vec4(boundMax.x, boundMin.y, boundMax.z, 1.0f));
		l_vertices[1].m_texCoord = Vec2(1.0f, 0.0f);

		l_vertices[2].m_pos = (Vec4(boundMin.x, boundMin.y, boundMax.z, 1.0f));
		l_vertices[2].m_texCoord = Vec2(0.0f, 0.0f);

		l_vertices[3].m_pos = (Vec4(boundMin.x, boundMax.y, boundMax.z, 1.0f));
		l_vertices[3].m_texCoord = Vec2(0.0f, 1.0f);

		l_vertices[4].m_pos = (Vec4(boundMax.x, boundMax.y, boundMin.z, 1.0f));
		l_vertices[4].m_texCoord = Vec2(1.0f, 1.0f);

		l_vertices[5].m_pos = (Vec4(boundMax.x, boundMin.y, boundMin.z, 1.0f));
		l_vertices[5].m_texCoord = Vec2(1.0f, 0.0f);

		l_vertices[6].m_pos = (Vec4(boundMin.x, boundMin.y, boundMin.z, 1.0f));
		l_vertices[6].m_texCoord = Vec2(0.0f, 0.0f);

		l_vertices[7].m_pos = (Vec4(boundMin.x, boundMax.y, boundMin.z, 1.0f));
		l_vertices[7].m_texCoord = Vec2(0.0f, 1.0f);

		for (auto& l_vertexData : l_vertices)
		{
			l_vertexData.m_normal = Vec4(l_vertexData.m_pos.x, l_vertexData.m_pos.y, l_vertexData.m_pos.z, 0.0f).normalize();
		}

		return l_vertices;
	}

	inline std::vector<Vertex> generateAABBVertices(const AABB& rhs)
	{
		auto boundMax = rhs.m_boundMax;
		auto boundMin = rhs.m_boundMin;

		return generateAABBVertices(boundMax, boundMin);
	}

	inline Vec4 colorTemperatureToRGB(float kelvin)
	{
		float temp = kelvin / 100.0f;

		float red, green, blue;

		if (temp <= 66.0f)
		{
			red = 255.0f;

			green = temp;
			green = 99.4708025861f * std::log(green) - 161.1195681661f;

			if (temp <= 19)
			{
				blue = 0.0f;
			}
			else
			{
				blue = temp - 10.0f;
				blue = 138.5177312231f * std::log(blue) - 305.0447927307f;
			}
		}
		else
		{
			red = temp - 60.0f;
			red = 329.698727446f * std::pow(red, -0.1332047592f);

			green = temp - 60.0f;
			green = 288.1221695283f * std::pow(green, -0.0755148492f);

			blue = 255.0f;
		}

		red /= 255.0f;
		green /= 255.0f;
		blue /= 255.0f;

		return Vec4(clamp(red, 0.0f, 1.0f), clamp(green, 0.0f, 1.0f), clamp(blue, 0.0f, 1.0f), 1.0f);
	}
}

using namespace InnoMath;