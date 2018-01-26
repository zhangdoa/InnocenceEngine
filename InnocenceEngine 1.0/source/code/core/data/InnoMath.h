#pragma once

//typedef __m128 __vec4;
//typedef __vec4 __vec3;
//typedef __vec4 __vec2;

const static double PI = 3.14159265358979323846264338327950288;

// [rowIndex][columnIndex]
class mat4
{
public:
	mat4();
	mat4(const mat4& rhs);
	mat4& operator=(const mat4& rhs);
	mat4 operator*(const mat4& rhs);
	// only for some semantically or literally usage
	mat4 operator*(float rhs);
	mat4 mul(const mat4& rhs);
	// only for some semantically or literally usage
	mat4 mul(float rhs);

	void initializeToPerspectiveMatrix(float FOV, float HWRatio, float zNear, float zFar);
	float m[4][4];
};

//the w component is the last one
class quat
{
public:
	quat();
	quat(float rhsX, float rhsY, float rhsZ, float rhsW);
	quat(const quat& rhs);
	quat& operator=(const quat& rhs);

	~quat();

	quat mul(const quat& rhs);
	// only for some semantically or literally usage
	quat mul(float rhs);
	float length();
	quat normalize();

	bool operator!=(const quat& rhs);
	bool operator==(const quat& rhs);

	mat4 toRotationMartix();

	float x;
	float y;
	float z;
	float w;
};

class vec3
{
public:
	vec3();
	vec3(float rhsX, float rhsY, float rhsZ);
	vec3(const vec3& rhs);
	vec3& operator=(const vec3& rhs);

	~vec3();

	vec3 add(const vec3& rhs);
	vec3 operator+(const vec3& rhs);
	vec3 add(float rhs);
	vec3 operator+(float rhs);
	vec3 sub(const vec3& rhs);
	vec3 operator-(const vec3& rhs);
	vec3 sub(float rhs);
	vec3 operator-(float rhs);
	float dot(const vec3& rhs);
	vec3 cross(const vec3& rhs);
	vec3 mul(const vec3& rhs);
	vec3 mul(float rhs);
	float length();
	vec3 normalize();

	bool operator!=(const vec3& rhs);
	bool operator==(const vec3& rhs);

	mat4 toTranslationMartix();
	mat4 toScaleMartix();

	float x;
	float y;
	float z;
};

class vec2
{
public:
	vec2();
	vec2(float rhsX, float rhsY);
	vec2(const vec2& rhs);
	vec2& operator=(const vec2& rhs);

	~vec2();

	vec2 add(const vec2& rhs);
	vec2 sub(const vec2& rhs);
	float dot(const vec2& rhs);
	float length();
	vec2 normalize();

	float x;
	float y;
};