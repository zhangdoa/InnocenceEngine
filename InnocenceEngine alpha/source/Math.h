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

	Vec3f operator+(const Vec3f& r) const;
	Vec3f operator+(float r) const;
	Vec3f operator-(const Vec3f& r) const;
	Vec3f operator-(float r) const;
	Vec3f operator*(const Vec3f& r) const;
	Vec3f operator*(float r) const;
	Vec3f operator/(const Vec3f& r) const;
	Vec3f operator/(float r) const;
	bool operator!=(const Vec3f& r) const;


	float max() const;
	float length() const;
	float dot(const Vec3f& r) const;
	Vec3f cross(const Vec3f& r) const;
	Vec3f normalized() const;
	Vec3f rotate(float angle) const;
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

class Quaternion
{
public:
	Quaternion();
	Quaternion(float x, float y, float z, float w);
	Quaternion(Vec3f axis, float angle);
	~Quaternion();

	float getX() const;
	float getY() const;
	float getZ() const;
	float getW() const;

	float length();
	Quaternion normalized();
	Quaternion conjugate();
	Quaternion operator* (const Quaternion& r);
	Quaternion operator* (const Vec3f& r);
	Quaternion operator* (float r);
	bool operator!=(const Quaternion& r) const;
	Mat4f toRotationMatrix();

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
	const Quaternion& getRot();
	const Vec3f& getScale();

	void setPos(const Vec3f& pos);
	void setRot(const Quaternion& rot);
	void setScale(const Vec3f& scale);

	const Vec3f& getOldPos();
	const Quaternion& getOldRot();
	const Vec3f& getOldScale();

private:

	Vec3f m_pos;
	Quaternion m_rot;
	Vec3f m_scale;

	Vec3f m_oldPos;
	Quaternion m_oldRot;
	Vec3f m_oldScale;
};


