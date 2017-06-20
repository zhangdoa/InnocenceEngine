#pragma once
#include "stdafx.h"
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

