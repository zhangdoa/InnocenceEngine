#pragma once
#include "common/stdafx.h"
//typedef __m128 __vec4;
//typedef __vec4 __vec3;
//typedef __vec4 __vec2;

const static double PI = 3.14159265358979323846264338327950288;

class vec3;

/*

matrix4x4 mathematical convention :
| a00 a01 a02 a03 |
| a10 a11 a12 a13 |
| a20 a21 a22 a23 |
| a30 a31 a32 a33 |

*/

/* Column-Major vector4 mathematical convention

vector4 :
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

vector4 :
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

/* Column-Major memory layout
[columnIndex][rowIndex]
| m[0][0] <-> a00 m[1][0] <-> a01 m[2][0] <-> a02 m[3][0] <-> a03 |
| m[0][1] <-> a10 m[1][1] <-> a11 m[2][1] <-> a12 m[3][1] <-> a13 |
| m[0][2] <-> a20 m[1][2] <-> a21 m[2][2] <-> a22 m[3][2] <-> a23 |
| m[0][3] <-> a30 m[1][3] <-> a31 m[2][3] <-> a32 m[3][3] <-> a33 |
*/

/* Row-Major memory layout
[rowIndex][columnIndex]
| m[0][0] <-> a00 m[0][1] <-> a01 m[0][2] <-> a02 m[0][3] <-> a03 |
| m[1][0] <-> a10 m[1][1] <-> a11 m[1][2] <-> a12 m[1][3] <-> a13 |
| m[2][0] <-> a20 m[2][1] <-> a21 m[2][2] <-> a22 m[2][3] <-> a23 |
| m[3][0] <-> a30 m[3][1] <-> a31 m[3][2] <-> a32 m[3][3] <-> a33 |
*/

// chose Row-Major vector4 mathematical convention and Column-Major memory layout in C/C++
class mat4
{
public:
	mat4();
	mat4(const mat4& rhs);
	mat4& operator=(const mat4& rhs);
	mat4 operator*(const mat4& rhs);
	// only for some semantically or literally usage
	mat4 operator*(double rhs);
	mat4 mul(const mat4& rhs);
	// only for some semantically or literally usage
	mat4 mul(double rhs);
	 
	void initializeToPerspectiveMatrix(double FOV, double HWRatio, double zNear, double zFar);
	void initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar);
	mat4 lookAt(const vec3& eyePos, const vec3& centerPos, const vec3& upDir);
	float m[4][4];
};

//the w component is the last one
class quat
{
public:
	quat();
	quat(double rhsX, double rhsY, double rhsZ, double rhsW);
	quat(const quat& rhs);
	quat& operator=(const quat& rhs);

	~quat();

	quat mul(const quat& rhs);
	// only for some semantically or literally usage
	quat mul(double rhs);
	double length();
	quat normalize();

	bool operator!=(const quat& rhs);
	bool operator==(const quat& rhs);

	mat4 toRotationMartix();

	double x;
	double y;
	double z;
	double w;
};

class vec3
{
public:
	vec3();
	vec3(double rhsX, double rhsY, double rhsZ);
	vec3(const vec3& rhs);
	vec3& operator=(const vec3& rhs);

	~vec3();

	vec3 add(const vec3& rhs);
	vec3 operator+(const vec3& rhs);
	vec3 add(double rhs);
	vec3 operator+(double rhs);
	vec3 sub(const vec3& rhs);
	vec3 operator-(const vec3& rhs);
	vec3 sub(double rhs);
	vec3 operator-(double rhs);
	double dot(const vec3& rhs);
	double operator*(const vec3& rhs);
	vec3 cross(const vec3& rhs);
	vec3 mul(const vec3& rhs);
	vec3 mul(double rhs);
	vec3 operator*(double rhs);
	double length();
	vec3 normalize();

	bool operator!=(const vec3& rhs);
	bool operator==(const vec3& rhs);

	mat4 toTranslationMartix();
	mat4 toScaleMartix();

	double x;
	double y;
	double z;
};

class vec2
{
public:
	vec2();
	vec2(double rhsX, double rhsY);
	vec2(const vec2& rhs);
	vec2& operator=(const vec2& rhs);

	~vec2();

	vec2 add(const vec2& rhs);
	vec2 sub(const vec2& rhs);
	double dot(const vec2& rhs);
	double length();
	vec2 normalize();

	double x;
	double y;
};

class Vertex
{
public:
	Vertex();
	Vertex(const Vertex& rhs);
	Vertex& operator=(const Vertex& rhs);
	Vertex(const vec3& pos, const vec2& texCoord, const vec3& normal);
	~Vertex();

	vec3 m_pos;
	vec2 m_texCoord;
	vec3 m_normal;
};

class Transform
{
public:
	Transform();
	~Transform();

	enum direction { FORWARD, BACKWARD, UP, DOWN, RIGHT, LEFT };
	void update();
	void rotate(const vec3 & axis, double angle);

	vec3& getPos();
	quat& getRot();
	vec3& getScale();

	void setPos(const vec3& pos);
	void setRot(const quat& rot);
	void setScale(const vec3& scale);

	vec3& getOldPos();
	quat& getOldRot();
	vec3& getOldScale();

	vec3 getDirection(direction direction) const;

private:
	vec3 m_pos;
	quat m_rot;
	vec3 m_scale;

	vec3 m_oldPos;
	quat m_oldRot;
	vec3 m_oldScale;
};

enum class visiblilityType { INVISIBLE, BILLBOARD, STATIC_MESH, SKYBOX, GLASSWARE };
// mesh custom types
enum class meshType { TWO_DIMENSION, THREE_DIMENSION };
enum class meshShapeType { QUAD, CUBE, SPHERE, CUSTOM };
enum class meshDrawMethod { TRIANGLE, TRIANGLE_STRIP };
// texture custom types
enum class textureType { INVISIBLE, NORMAL, ALBEDO, METALLIC, ROUGHNESS, AMBIENT_OCCLUSION, CUBEMAP, ENVIRONMENT_CAPTURE, ENVIRONMENT_CONVOLUTION, ENVIRONMENT_PREFILTER, EQUIRETANGULAR, RENDER_BUFFER_SAMPLER };
enum class textureColorComponentsFormat { RED, RG, RGB, RGBA, R8, RG8, RGB8, RGBA8, R16, RG16, RGB16, RGBA16, R16F, RG16F, RGB16F, RGBA16F, R32F, RG32F, RGB32F, RGBA32F, SRGB, SRGBA, SRGB8, SRGBA8 };
enum class texturePixelDataFormat { RED, RG, RGB, RGBA };
enum class texturePixelDataType { UNSIGNED_BYTE, BYTE, UNSIGNED_SHORT, SHORT, UNSIGNED_INT, INT, FLOAT };
enum class textureWrapMethod { CLAMP_TO_EDGE, REPEAT };
enum class textureFilterMethod { NEAREST, LINEAR, LINEAR_MIPMAP_LINEAR };
// shader custom types
enum class shaderType { VERTEX, GEOMETRY, FRAGMENT };
using shaderFilePath = std::string;
using shaderCodeContent = std::string;
using shaderCodeContentPair = std::pair<shaderFilePath, shaderCodeContent>;
using shaderData = std::pair<shaderType, shaderCodeContentPair>;
// frame and render buffer custom types
enum class frameBufferType { FORWARD, DEFER, SHADOWMAP, CUBEMAP };
enum class renderBufferType { DEPTH, STENCIL, DEPTH_AND_STENCIL };

using textureID = unsigned long int;
using meshID = unsigned long int;
using texturePair = std::pair<textureType, textureID>;
using textureMap = std::unordered_map<textureType, textureID>;
using modelPair = std::pair<meshID, textureMap>;
using modelMap = std::unordered_map<meshID, textureMap>;

using textureFileNamePair = std::pair<textureType, std::string>;
using textureFileNameMap = std::unordered_map<textureType, std::string>;