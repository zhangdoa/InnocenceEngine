#pragma once
#include "common/stdafx.h"
//typedef __m128 __vec4;
//typedef __vec4 __vec3;
//typedef __vec4 __vec2;

const static double PI = 3.14159265358979323846264338327950288;

class vec3;
// [rowIndex][columnIndex]
// Column-major order
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