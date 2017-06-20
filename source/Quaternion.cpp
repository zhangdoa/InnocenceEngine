#include "Quaternion.h"

Quaternion::Quaternion()
{
	_x = 0.0f;
	_y = 0.0f;
	_z = 0.0f;
	_w = 0.0f;
}

Quaternion::Quaternion(float x, float y, float z, float w)
{
	_x = x;
	_y = y;
	_z = z;
	_w = w;

}

Quaternion::Quaternion(Vec3f axis, float angle)
{
	float sinHalfAngle = sinf(angle / 2);
	float cosHalfAngle = cosf(angle / 2);

	_x = axis.getX() * sinHalfAngle;
	_y = axis.getY() * sinHalfAngle;
	_z = axis.getZ() * sinHalfAngle;
	_w = cosHalfAngle;

}

Quaternion::~Quaternion()
{
}

float Quaternion::length()
{
	return sqrtf(_x * _x + _y * _y + _z * _z + _w * _w);
}

Quaternion Quaternion::normalized()
{
	return Quaternion();
}

Quaternion Quaternion::conjugate()
{
	return Quaternion(-_x, -_y, -_z, -_w);
}

Quaternion Quaternion::operator*(const Quaternion & r)
{
	float w_ = _w * r._w - _x * r._x - _y * r._y - _z * r._z;
	float x_ = _x * r._w + _w * r._x + _y * r._z - _z * r._y;
	float y_ = _y * r._w + _w * r._y + _z * r._x - _x * r._z;
	float z_ = _z * r._w + _w * r._z + _x * r._y - _y * r._x;

	return Quaternion(x_, y_, z_, w_);
}

Quaternion Quaternion::operator*(Vec3f & r)
{
	float w_ = -_x * r.getX() - _y * r.getY() - _z * r.getZ();
	float x_ = _w * r.getX() + _y * r.getZ() - _z * r.getY();
	float y_ = _w * r.getY() + _z * r.getX() - _x * r.getZ();
	float z_ = _w * r.getZ() + _x * r.getY() - _y * r.getX();

	return Quaternion(x_, y_, z_, w_);
}

Quaternion Quaternion::operator*(float r)
{
	return Quaternion(_x * r, _y * r, _z * r, _w * r);
}

Mat4f Quaternion::toRotationMatrix() const
{
	Vec3f forward = Vec3f(2.0f * (_x * _z - _w * _y), 2.0f * (_y * _z + _w * _x), 1.0f - 2.0f * (_x * _x + _y * _y));
	Vec3f up = Vec3f(2.0f * (_x * _y + _w * _z), 1.0f - 2.0f * (_x * _x + _z * _z), 2.0f * (_y * _z - _w * _x));
	Vec3f right = Vec3f(1.0f - 2.0f * (_y * _y + _z * _z), 2.0f * (_x * _y - _w * _z), 2.0f * (_x * _z + _w * _y));

	return Mat4f().initRotation(forward, up, right);
}
