#pragma once
#include "stdafx.h"
#define PI 3.14159265

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

	Vec2f operator+(const Vec2f& r) const;
	Vec2f operator+(float r) const;
	Vec2f operator-(const Vec2f& r) const;
	Vec2f operator-(float r) const;
	Vec2f operator*(const Vec2f& r) const;
	Vec2f operator*(float r) const;
	Vec2f operator/(const Vec2f& r) const;
	Vec2f operator/(float r) const;
	bool operator!=(const Vec2f& r) const;

	float getMaxElem() const;
	float getLength() const;
	float dot(const Vec2f& r) const;
	float cross(const Vec2f& r) const;
	void normalize();
	Vec2f getNormalizedVec2f() const;
	void rotate(float angle);
	Vec2f getRotatedVec2f(float angle) const;
	Vec2f lerp(const Vec2f& dest, float lerpFactor) const;


private:
	float _x;
	float _y;
};

class Vec4f;

class Vec3f
{
public:
	Vec3f();
	Vec3f(float x, float y, float z);
	~Vec3f();

	float getX() const;
	float getY() const;
	float getZ() const;

	Vec3f operator+(const Vec3f& r) const;
	Vec3f operator+(float r) const;
	Vec3f operator-(const Vec3f& r) const;
	Vec3f operator-(float r) const;
	Vec3f operator*(const Vec3f& r) const;
	Vec3f operator*(float r) const;
	Vec3f operator/(const Vec3f& r) const;
	Vec3f operator/(float r) const;
	bool operator!=(const Vec3f& r) const;

	float getMaxElem() const;
	float getLength() const;
	float dot(const Vec3f& r) const;
	Vec3f cross(const Vec3f& r) const;
	void normalize();
	Vec3f getNormalizedVec3f() const;
	void rotate(const Vec3f& axis, float angle);
	void rotate(const Vec4f& rotation);
	Vec3f getRotatedVec3f(const Vec3f& axis, float angle) const;
	Vec3f getRotatedVec3f(const Vec4f& rotation) const;
	Vec3f lerp(const Vec3f& dest, float lerpFactor) const;

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

class Vec4f
{
public:
	Vec4f();
	Vec4f(float x, float y, float z, float w);
	Vec4f(const Vec3f& axis, float angle);
	~Vec4f();

	float getX() const;
	float getY() const;
	float getZ() const;
	float getW() const;


	Vec4f operator* (const Vec4f& r) const;
	Vec4f operator* (const Vec3f& r) const;
	Vec4f operator* (float r) const;
	bool operator!=(const Vec4f& r) const;



	float getMaxElem() const;
	float getLength() const;
	float dot(const Vec4f& r) const;
	Vec4f cross(const Vec4f& r) const;
	void normalize();
	Vec4f getNormalizedVec4f() const;
	void rotate(float angle);
	Vec4f getRotatedVec4f(float angle) const;
	Vec4f getRotatedVec4f(const Vec4f& rotation) const;
	Vec4f lerp(const Vec4f& dest, float lerpFactor) const;
	void conjugate();
	Vec4f getConjugatedVec4f() const;
	Mat4f toRotationMatrix() const;

	Vec3f getForward() const;
	Vec3f getBackward() const;
	Vec3f getUp() const;
	Vec3f getDown() const;
	Vec3f getRight() const;
	Vec3f getLeft() const;

private:
	float _x;
	float _y;
	float _z;
	float _w;
};


class Transform
{
public:
	Transform();
	~Transform();

	void update();
	void rotate(Vec3f axis, float angle);

	const Vec3f& getPos();
	const Vec4f& getRot();
	const Vec3f& getScale();

	void setPos(const Vec3f& pos);
	void setRot(const Vec4f& rot);
	void setScale(const Vec3f& scale);

	const Vec3f& getOldPos();
	const Vec4f& getOldRot();
	const Vec3f& getOldScale();

private:

	Vec3f _pos;
	Vec4f _rot;
	Vec3f _scale;

	Vec3f _oldPos;
	Vec4f _oldRot;
	Vec3f _oldScale;
};


