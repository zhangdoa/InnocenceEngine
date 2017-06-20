#include "Transform.h"



Transform::Transform()
{
	_parentMat.initIdentity();
	_pos = Vec3f(0.0f, 0.0f, 0.0f);
	_rot = Quaternion(0.0f, 0.0f, 0.0f, 0.0f);
	_scale = Vec3f(1.0f, 1.0f, 1.0f);
}


Transform::~Transform()
{
}

void Transform::update()
{
	if (&_oldPos != nullptr)
	{
		_oldPos = _pos;
		_oldRot = _rot;
		_oldScale = _scale;
	}
	else
	{
		_oldPos = _pos + 1.0f;
		_oldRot = _rot * 0.5f;
		_oldScale = _scale + 1.0f;
	}
}

void Transform::setParent(Transform * parentTransform)
{
	_parent = parentTransform;
}

void Transform::rotate(Vec3f axis, float angle)
{
	_rot = Quaternion(axis, angle) * _rot.normalized();
}

bool Transform::hasChanged()
{
	if (&_pos != &_oldPos || &_rot != &_oldRot || &_scale != &_oldScale)
	{
		return true;
	}

	if (_parent != nullptr && _parent->hasChanged())
	{
		return true;
	}
	return false;
}

Mat4f Transform::getTransformation()
{
	Mat4f translationMatrix = Mat4f().initTranslation(_pos.getX(), _pos.getY(), _pos.getZ());
	Mat4f rotaionMartix = _rot.toRotationMatrix();
	Mat4f scaleMartix = Mat4f().initScale(_scale.getX(), _scale.getY(), _scale.getZ());

	return getParentMatrix() * translationMatrix * rotaionMartix * scaleMartix;
}

Mat4f Transform::getParentMatrix()
{
	if (_parent != nullptr && _parent->hasChanged())
	{
		_parentMat = _parent->getTransformation();
	}
	return _parentMat;
}

Vec3f Transform::getTransformedPos()
{
	return getParentMatrix().transform(_pos);
}

Quaternion Transform::getTransformedRot()
{
	Quaternion parentRotation = Quaternion(0, 0, 0, 1);
	if (_parent != nullptr)
		parentRotation = _parent->getTransformedRot();

	return parentRotation * _rot;
}
