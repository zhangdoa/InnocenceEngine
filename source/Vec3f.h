#pragma once
#include "stdafx.h"
class Vec3f
{
public:
	Vec3f();
	Vec3f(float x, float y, float z);
	~Vec3f();

	float getX() const;
	float getY() const;
	float getZ() const;

	Vec3f operator+(const Vec3f& r);
	Vec3f operator+(float r);
	Vec3f operator-(const Vec3f& r);
	Vec3f operator-(float r);
	Vec3f operator*(const Vec3f& r);
	Vec3f operator*(float r);
	Vec3f operator/(const Vec3f& r);
	Vec3f operator/(float r);


	float max();
	float length();
	float dot(const Vec3f& r);
	Vec3f cross(const Vec3f& r);
	Vec3f normalized();
	Vec3f rotate(float angle);
	Vec3f lerp(const Vec3f& dest, float lerpFactor);


private:
	float _x;
	float _y;
	float _z;
};

