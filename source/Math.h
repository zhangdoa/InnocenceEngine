#pragma once
#include "stdafx.h"

class Math
{
public:
	Math();
	~Math();
};

class Vec2f
{
public:
	Vec2f();
	Vec2f(float x, float y);
	~Vec2f();

	float getX() const;
	float getY() const;

	Vec2f operator+(const Vec2f& r);
	Vec2f operator+(float r);
	Vec2f operator-(const Vec2f& r);
	Vec2f operator-(float r);
	Vec2f operator*(const Vec2f& r);
	Vec2f operator*(float r);
	Vec2f operator/(const Vec2f& r);
	Vec2f operator/(float r);


	float max();
	float length();
	float dot(const Vec2f& r);
	float cross(const Vec2f& r);
	Vec2f normalized();
	Vec2f rotate(float angle);
	Vec2f lerp(const Vec2f& dest, float lerpFactor);


private:
	float _x;
	float _y;
};

class Vec3f
{
public:
	Vec3f();
	Vec3f(float x, float y, float z);
	~Vec3f();

	float getX() const;
	float getY() const;
	float getZ() const;

	Vec3f operator+(Vec3f* r);
	Vec3f operator+(float r);
	Vec3f operator-(Vec3f* r);
	Vec3f operator-(float r);
	Vec3f operator*(Vec3f* r);
	Vec3f operator*(float r);
	Vec3f operator/(Vec3f*r);
	Vec3f operator/(float r);


	float max();
	float length();
	float dot(Vec3f* r);
	Vec3f cross(Vec3f* r);
	Vec3f normalized();
	Vec3f rotate(float angle);
	Vec3f lerp(Vec3f* dest, float lerpFactor);


private:
	float _x;
	float _y;
	float _z;
};

class Mat4f
{
public:
	Mat4f();
	~Mat4f();
	void initIdentity();
	void initTranslation(float x, float y, float z);
	void initRotation(float x, float y, float z);
	void initRotation(Vec3f forward, Vec3f up);
	void initRotation(Vec3f forward, Vec3f up, Vec3f right);
	void initScale(float x, float y, float z);
	void initPerspective(float fov, float aspectRatio, float zNear, float zFar);
	void initOrthographic(float left, float right, float bottom, float top, float near, float far);
	Mat4f operator* (const Mat4f& r);
	Vec3f transform(Vec3f r);
	float getElem(int x, int y) const;
	void setElem(int x, int y, float value);
	float* getAllElem();
	void setAllElem(float r[4][4]);
private:
	float m[4][4];

};

class Quaternion
{
public:
	Quaternion();
	Quaternion(float x, float y, float z, float w);
	Quaternion(Vec3f axis, float angle);
	~Quaternion();

	float length();
	Quaternion normalized();
	Quaternion conjugate();
	Quaternion operator* (Quaternion* r);
	Quaternion operator* (Vec3f* r);
	Quaternion operator* (float r);
	Mat4f toRotationMatrix() const;

private:
	float _x;
	float _y;
	float _z;
	float _w;
};




