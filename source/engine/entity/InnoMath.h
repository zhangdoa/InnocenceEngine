#pragma once
#include "common/stdafx.h"
#include "common/config.h"
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

/* Column-Major memory layout (in C/C++)
[columnIndex][rowIndex]
| m[0][0] <-> a00 m[1][0] <-> a01 m[2][0] <-> a02 m[3][0] <-> a03 |
| m[0][1] <-> a10 m[1][1] <-> a11 m[2][1] <-> a12 m[3][1] <-> a13 |
| m[0][2] <-> a20 m[1][2] <-> a21 m[2][2] <-> a22 m[3][2] <-> a23 |
| m[0][3] <-> a30 m[1][3] <-> a31 m[2][3] <-> a32 m[3][3] <-> a33 |
*/

/* Row-Major memory layout (in C/C++)
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
	vec2 operator+(const vec2& rhs);
	vec2 add(double rhs);
	vec2 operator+(double rhs);
	vec2 sub(const vec2& rhs);
	vec2 operator-(const vec2& rhs);
	vec2 sub(double rhs);
	vec2 operator-(double rhs);
	double dot(const vec2& rhs);
	double operator*(const vec2& rhs);
	vec2 cross(const vec2& rhs);
	vec2 mul(const vec2& rhs);
	vec2 mul(double rhs);
	vec2 operator*(double rhs);
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

class AABB
{
public:
	AABB();
	AABB(const AABB& rhs);
	AABB& operator=(const AABB& rhs);
	AABB(const vec3& center, const vec3& halfWidths);
	~AABB();

	vec3 m_center;
	vec3 m_halfWidths;
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
enum class textureType { INVISIBLE, NORMAL, ALBEDO, METALLIC, ROUGHNESS, AMBIENT_OCCLUSION, CUBEMAP, ENVIRONMENT_CAPTURE, ENVIRONMENT_CONVOLUTION, ENVIRONMENT_PREFILTER, EQUIRETANGULAR, RENDER_BUFFER_SAMPLER, SHADOWMAP };
enum class textureColorComponentsFormat { RED, RG, RGB, RGBA, R8, RG8, RGB8, RGBA8, R16, RG16, RGB16, RGBA16, R16F, RG16F, RGB16F, RGBA16F, R32F, RG32F, RGB32F, RGBA32F, SRGB, SRGBA, SRGB8, SRGBA8, DEPTH_COMPONENT };
enum class texturePixelDataFormat { RED, RG, RGB, RGBA, DEPTH_COMPONENT };
enum class texturePixelDataType { UNSIGNED_BYTE, BYTE, UNSIGNED_SHORT, SHORT, UNSIGNED_INT, INT, FLOAT };
enum class textureWrapMethod { CLAMP_TO_EDGE, REPEAT, CLAMP_TO_BORDER };
enum class textureFilterMethod { NEAREST, LINEAR, LINEAR_MIPMAP_LINEAR };
// shader custom types
enum class shaderType { VERTEX, GEOMETRY, FRAGMENT };
using shaderFilePath = std::string;
using shaderCodeContent = std::string;
using shaderCodeContentPair = std::pair<shaderFilePath, shaderCodeContent>;
using shaderData = std::pair<shaderType, shaderCodeContentPair>;
// frame and render buffer custom types
enum class frameBufferType { FORWARD, DEFER, SHADOW_PASS, ENVIRONMENT_PASS };
enum class renderBufferType { NONE, DEPTH, STENCIL, DEPTH_AND_STENCIL };

using textureID = unsigned long int;
using meshID = unsigned long int;
using texturePair = std::pair<textureType, textureID>;
using textureMap = std::unordered_map<textureType, textureID>;
using modelPair = std::pair<meshID, textureMap>;
using modelMap = std::unordered_map<meshID, textureMap>;
using textureFileNamePair = std::pair<textureType, std::string>;
using textureFileNameMap = std::unordered_map<textureType, std::string>;

#define INNO_KEY_SPACE              32
#define INNO_KEY_APOSTROPHE         39  /* ' */
#define INNO_KEY_COMMA              44  /* , */
#define INNO_KEY_MINUS              45  /* - */
#define INNO_KEY_PERIOD             46  /* . */
#define INNO_KEY_SLASH              47  /* / */
#define INNO_KEY_0                  48
#define INNO_KEY_1                  49
#define INNO_KEY_2                  50
#define INNO_KEY_3                  51
#define INNO_KEY_4                  52
#define INNO_KEY_5                  53
#define INNO_KEY_6                  54
#define INNO_KEY_7                  55
#define INNO_KEY_8                  56
#define INNO_KEY_9                  57
#define INNO_KEY_SEMICOLON          59  /* ; */
#define INNO_KEY_EQUAL              61  /* = */
#define INNO_KEY_A                  65
#define INNO_KEY_B                  66
#define INNO_KEY_C                  67
#define INNO_KEY_D                  68
#define INNO_KEY_E                  69
#define INNO_KEY_F                  70
#define INNO_KEY_G                  71
#define INNO_KEY_H                  72
#define INNO_KEY_I                  73
#define INNO_KEY_J                  74
#define INNO_KEY_K                  75
#define INNO_KEY_L                  76
#define INNO_KEY_M                  77
#define INNO_KEY_N                  78
#define INNO_KEY_O                  79
#define INNO_KEY_P                  80
#define INNO_KEY_Q                  81
#define INNO_KEY_R                  82
#define INNO_KEY_S                  83
#define INNO_KEY_T                  84
#define INNO_KEY_U                  85
#define INNO_KEY_V                  86
#define INNO_KEY_W                  87
#define INNO_KEY_X                  88
#define INNO_KEY_Y                  89
#define INNO_KEY_Z                  90
#define INNO_KEY_LEFT_BRACKET       91  /* [ */
#define INNO_KEY_BACKSLASH          92  /* \ */
#define INNO_KEY_RIGHT_BRACKET      93  /* ] */
#define INNO_KEY_GRAVE_ACCENT       96  /* ` */
#define INNO_KEY_WORLD_1            161 /* non-US #1 */
#define INNO_KEY_WORLD_2            162 /* non-US #2 */

/* Function keys */
#define INNO_KEY_ESCAPE             256
#define INNO_KEY_ENTER              257
#define INNO_KEY_TAB                258
#define INNO_KEY_BACKSPACE          259
#define INNO_KEY_INSERT             260
#define INNO_KEY_DELETE             261
#define INNO_KEY_RIGHT              262
#define INNO_KEY_LEFT               263
#define INNO_KEY_DOWN               264
#define INNO_KEY_UP                 265
#define INNO_KEY_PAGE_UP            266
#define INNO_KEY_PAGE_DOWN          267
#define INNO_KEY_HOME               268
#define INNO_KEY_END                269
#define INNO_KEY_CAPS_LOCK          280
#define INNO_KEY_SCROLL_LOCK        281
#define INNO_KEY_NUM_LOCK           282
#define INNO_KEY_PRINT_SCREEN       283
#define INNO_KEY_PAUSE              284
#define INNO_KEY_F1                 290
#define INNO_KEY_F2                 291
#define INNO_KEY_F3                 292
#define INNO_KEY_F4                 293
#define INNO_KEY_F5                 294
#define INNO_KEY_F6                 295
#define INNO_KEY_F7                 296
#define INNO_KEY_F8                 297
#define INNO_KEY_F9                 298
#define INNO_KEY_F10                299
#define INNO_KEY_F11                300
#define INNO_KEY_F12                301
#define INNO_KEY_F13                302
#define INNO_KEY_F14                303
#define INNO_KEY_F15                304
#define INNO_KEY_F16                305
#define INNO_KEY_F17                306
#define INNO_KEY_F18                307
#define INNO_KEY_F19                308
#define INNO_KEY_F20                309
#define INNO_KEY_F21                310
#define INNO_KEY_F22                311
#define INNO_KEY_F23                312
#define INNO_KEY_F24                313
#define INNO_KEY_F25                314
#define INNO_KEY_KP_0               320
#define INNO_KEY_KP_1               321
#define INNO_KEY_KP_2               322
#define INNO_KEY_KP_3               323
#define INNO_KEY_KP_4               324
#define INNO_KEY_KP_5               325
#define INNO_KEY_KP_6               326
#define INNO_KEY_KP_7               327
#define INNO_KEY_KP_8               328
#define INNO_KEY_KP_9               329
#define INNO_KEY_KP_DECIMAL         330
#define INNO_KEY_KP_DIVIDE          331
#define INNO_KEY_KP_MULTIPLY        332
#define INNO_KEY_KP_SUBTRACT        333
#define INNO_KEY_KP_ADD             334
#define INNO_KEY_KP_ENTER           335
#define INNO_KEY_KP_EQUAL           336
#define INNO_KEY_LEFT_SHIFT         340
#define INNO_KEY_LEFT_CONTROL       341
#define INNO_KEY_LEFT_ALT           342
#define INNO_KEY_LEFT_SUPER         343
#define INNO_KEY_RIGHT_SHIFT        344
#define INNO_KEY_RIGHT_CONTROL      345
#define INNO_KEY_RIGHT_ALT          346
#define INNO_KEY_RIGHT_SUPER        347
#define INNO_KEY_MENU               348
