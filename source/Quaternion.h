#pragma once
#include "stdafx.h"
#include "Vec3f.h"
#include "Mat4f.h"

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
	Quaternion operator* (const Quaternion& r);
	Quaternion operator* (Vec3f& r);
	Quaternion operator* (float r);
	Mat4f toRotationMatrix() const;

private:
	float _x;
	float _y;
	float _z;
	float _w;
};

