#pragma once
#include "stdafx.h"
#include "Vec3f.h"
class Mat4f
{
public:
	Mat4f();
	~Mat4f();
	Mat4f initIdentity();
	Mat4f initTranslation(float x, float y, float z);
	Mat4f initRotation(float x, float y, float z);
	Mat4f initRotation(Vec3f forward, Vec3f up);
	Mat4f initRotation(Vec3f forward, Vec3f up, Vec3f right);
	Mat4f initScale(float x, float y, float z);
	Mat4f initPerspective(float fov, float aspectRatio, float zNear, float zFar);
	Mat4f initOrthographic(float left, float right, float bottom, float top, float near, float far);
	Mat4f operator* (const Mat4f& r);
	Vec3f transform(Vec3f r);
	float getElem(int x, int y) const;
	void setElem(int x, int y, float value);
	float* getAllElem();
	void setAllElem(float r[4][4]);
private:
	float m[4][4];

};

