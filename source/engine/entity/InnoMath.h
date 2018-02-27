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