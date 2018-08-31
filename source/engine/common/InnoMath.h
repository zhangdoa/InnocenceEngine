#pragma once
#include "../common/stdafx.h"
#include "../common/config.h"

//typedef __m128 vec4;

const static double PI = 3.14159265358979323846264338327950288;

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

class mat4
{
public:
	mat4();
	mat4(const mat4& rhs);
	mat4& operator=(const mat4& rhs);
	mat4 operator*(const mat4& rhs);
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	vec4 operator*(const vec4& rhs);
#endif
	mat4 operator*(double rhs);
	mat4 transpose();
	mat4 inverse();
	double getDeterminant();
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
	void initializeToIdentityMatrix();
	void initializeToPerspectiveMatrix(double FOV, double WHRatio, double zNear, double zFar);
	void initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar);
	mat4 lookAt(const vec4& eyePos, const vec4& centerPos, const vec4& upDir);
	float m[4][4];
};

// In Homogeneous Coordinates, the w component is a scalar of x, y and z, to represent 3D point vector in 4D, set w to 1.0; to represent 3D direction vector in 4D, set w to 0.0.
// In Quaternion, the w component is sin(theta / 2).
class vec4
{
public:
	vec4();
	vec4(double rhsX, double rhsY, double rhsZ, double rhsW);
	vec4(const vec4& rhs);
	vec4& operator=(const vec4& rhs);

	~vec4();

	vec4 operator+(const vec4& rhs) const;
	vec4 operator+(double rhs) const;
	vec4 operator-(const vec4& rhs) const;
	vec4 operator-(double rhs) const;
	double operator*(const vec4& rhs) const;
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	vec4 operator*(const mat4 & rhs) const;
#endif
	vec4 cross(const vec4& rhs) const;
	vec4 scale(const vec4& rhs) const;
	vec4 scale(double rhs) const;
	vec4 operator*(double rhs) const;
	vec4 operator/(double rhs) const;
	vec4 quatMul(const vec4& rhs) const;
	vec4 quatMul(double rhs) const;
	vec4 quatConjugate() const;
	vec4 reciprocal() const;
	double length() const;
	vec4 normalize() const;
	vec4 lerp(const vec4& a, const vec4& b, double alpha) const;
	vec4 slerp(const vec4& a, const vec4& b, double alpha) const;
	vec4 nlerp(const vec4& a, const vec4& b, double alpha) const;

	bool operator!=(const vec4& rhs);
	bool operator==(const vec4& rhs);

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
	mat4 toTranslationMatrix();
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
	mat4 toRotationMatrix();
	mat4 toScaleMatrix();

	double x;
	double y;
	double z;
	double w;
};

class vec2
{
public:
	vec2();
	vec2(double rhsX, double rhsY);
	vec2(const vec2& rhs);
	vec2& operator=(const vec2& rhs);

	~vec2();

	vec2 operator+(const vec2& rhs);
	vec2 operator+(double rhs);
	vec2 operator-(const vec2& rhs);
	vec2 operator-(double rhs);
	double operator*(const vec2& rhs);
	vec2 scale(const vec2& rhs);
	vec2 scale(double rhs);
	vec2 operator*(double rhs);
	vec2 operator/(double rhs);
	double length();
	vec2 normalize();

	double x;
	double y;
};

class Vertex
{
public:
	Vertex() :
		m_pos(vec4(0.0, 0.0, 0.0, 1.0)),
		m_texCoord(vec2(0.0, 0.0)),
		m_normal(vec4(0.0, 0.0, 1.0, 0.0)) {};
	Vertex(const Vertex& rhs) :
		m_pos(rhs.m_pos),
		m_texCoord(rhs.m_texCoord),
		m_normal(rhs.m_normal) {};
	Vertex(const vec4& pos, const vec2& texCoord, const vec4& normal) :
		m_pos(pos),
		m_texCoord(texCoord),
		m_normal(normal) {};
	Vertex& operator=(const Vertex& rhs);
	~Vertex() {};

	vec4 m_pos;
	vec2 m_texCoord;
	vec4 m_normal;
};

class Ray
{
public:
	Ray():
		m_origin(vec4(0.0, 0.0, 0.0, 1.0)),
		m_direction(vec4(0.0, 0.0, 0.0, 0.0)) {};
	Ray(const Ray& rhs) :
		m_origin(rhs.m_origin),
		m_direction(rhs.m_direction) {};
	Ray& operator=(const Ray& rhs);
	~Ray() {};

	vec4 m_origin;
	vec4 m_direction;
};

class AABB
{
public:
	AABB() :
		m_center(vec4(0.0, 0.0, 0.0, 1.0)),
		m_sphereRadius(0.0),
		m_boundMin(vec4(0.0, 0.0, 0.0, 1.0)),
		m_boundMax(vec4(0.0, 0.0, 0.0, 1.0)) {};;
	AABB(const AABB& rhs) :
		m_center(rhs.m_center),
		m_sphereRadius(rhs.m_sphereRadius),
		m_boundMin(rhs.m_boundMin),
		m_boundMax(rhs.m_boundMax),
		m_vertices(rhs.m_vertices),
		m_indices(rhs.m_indices) {};
	AABB& operator=(const AABB& rhs);
	~AABB() {};

	vec4 m_center;
	double m_sphereRadius;
	vec4 m_boundMin;
	vec4 m_boundMax;

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
		m_pos(vec4(0.0, 0.0, 0.0, 1.0)),
		m_rot(vec4(0.0, 0.0, 0.0, 1.0)),
		m_scale(vec4(1.0, 1.0, 1.0, 1.0)),
		m_previousPos((m_pos + (1.0)) / 2.0),
		m_previousRot(m_rot.quatMul(vec4(1.0, 1.0, 1.0, 0.0))),
		m_previousScale(m_scale + (1.0)) {};
	~Transform() {};

	bool hasChanged();
	void update();
	void rotateInLocal(const vec4 & axis, double angle);

	vec4& getPos();
	vec4& getRot();
	vec4& getScale();

	void setLocalPos(const vec4& pos);
	void setLocalRot(const vec4& rot);
	void setLocalScale(const vec4& scale);

	mat4 caclLocalTranslationMatrix();
	mat4 caclLocalRotMatrix();
	mat4 caclLocalScaleMatrix();
	mat4 caclLocalTransformationMatrix();
	mat4 caclPreviousLocalTranslationMatrix();
	mat4 caclPreviousLocalRotMatrix();
	mat4 caclPreviousLocalScaleMatrix();
	mat4 caclPreviousLocalTransformationMatrix();

	vec4 caclGlobalPos();
	vec4 caclGlobalRot();
	vec4 caclGlobalScale();
	vec4 caclPreviousGlobalPos();
	vec4 caclPreviousGlobalRot();
	vec4 caclPreviousGlobalScale();

	mat4 caclGlobalTranslationMatrix();
	mat4 caclGlobalRotMatrix();
	mat4 caclGlobalScaleMatrix();
	mat4 caclGlobalTransformationMatrix();
	mat4 caclPreviousGlobalTranslationMatrix();
	mat4 caclPreviousGlobalRotMatrix();
	mat4 caclPreviousGlobalScaleMatrix();
	mat4 caclPreviousGlobalTransformationMatrix();

	mat4 caclLookAtMatrix();
	mat4 caclPreviousLookAtMatrix();

	mat4 getInvertLocalTranslationMatrix();
	mat4 getInvertLocalRotMatrix();
	mat4 getInvertLocalScaleMatrix();
	mat4 getInvertGlobalTranslationMatrix();
	mat4 getInvertGlobalRotMatrix();
	mat4 getInvertGlobalScaleMatrix();
	mat4 getPreviousInvertLocalTranslationMatrix();
	mat4 getPreviousInvertLocalRotMatrix();
	mat4 getPreviousInvertLocalScaleMatrix();
	mat4 getPreviousInvertGlobalTranslationMatrix();
	mat4 getPreviousInvertGlobalRotMatrix();
	mat4 getPreviousInvertGlobalScaleMatrix();
	vec4 getDirection(direction direction) const;
	vec4 getPreviousDirection(direction direction) const;
	Transform* m_parentTransform;

private:
	vec4 m_pos;
	vec4 m_rot;
	vec4 m_scale;

	vec4 m_previousPos;
	vec4 m_previousRot;
	vec4 m_previousScale;
};
