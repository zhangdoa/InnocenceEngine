#pragma once
#include "stdafx.h"
#include "Vec3f.h"
#include "Mat4f.h"
#include "Quaternion.h"
class Transform
{
public:
	Transform();
	~Transform();
	void update();
	void setParent(Transform* parent);
	void rotate(Vec3f axis, float angle);
	bool hasChanged();
	Mat4f getTransformation();
	Mat4f getParentMatrix();
	Vec3f getTransformedPos();
	Quaternion getTransformedRot();

private:
	Transform* _parent;
	Mat4f _parentMat;

	Vec3f _pos;
	Quaternion _rot;
	Vec3f _scale;

	Vec3f _oldPos;
	Quaternion _oldRot;
	Vec3f _oldScale;



};

