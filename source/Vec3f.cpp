#include "Vec3f.h"



Vec3f::Vec3f()
{
	_x = 0.0f;
	_y = 0.0f;
	_z = 0.0f;
}

Vec3f::Vec3f(float x, float y, float z)
{
	_x = x;
	_y = y;
	_z = z;
	
}


Vec3f::~Vec3f()
{
}

float Vec3f::getX() const
{
	return _x;
}

float Vec3f::getY() const
{
	return _y;
}

float Vec3f::getZ() const
{
	return _z;
}

Vec3f Vec3f::operator+(const Vec3f & r)
{
	return Vec3f(_x + r._x, _y + r._y, _z + r._z);
}

Vec3f Vec3f::operator+(float r)
{
	return Vec3f(_x + r, _y + r, _z + r);
}

Vec3f Vec3f::operator-(const Vec3f & r)
{
	return Vec3f(_x - r._x, _y - r._y, _z - r._z);
}

Vec3f Vec3f::operator-(float r)
{
	return Vec3f(_x - r, _y - r, _z - r);
}

Vec3f Vec3f::operator*(const Vec3f & r)
{
	return Vec3f(_x * r._x, _y * r._y, _z * r._z);
}

Vec3f Vec3f::operator*(float r)
{
	return Vec3f(_x * r, _y * r, _z * r);
}

Vec3f Vec3f::operator/(const Vec3f & r)
{
	return Vec3f(_x / r._x, _y / r._y, _z / r._z);
}

Vec3f Vec3f::operator/(float r)
{
	return Vec3f(_x * r, _y * r, _z * r);
}

float Vec3f::max()
{
	if (_x >= _y && _x >= _z)
	{
		return _x;
	}
	else if(_y >= _x && _y >= _z)
	{
		return _y;
	}
	return _z;
}

float Vec3f::length()
{
	return sqrtf(_x * _x + _y * _y + _z * _z);
}

float Vec3f::dot(const Vec3f & r)
{
	return _x * r._x + _y * r._y + _z * r._z;
}

Vec3f Vec3f::cross(const Vec3f & r)
{
	return Vec3f(_y * r._z - _z * r._y, _z * r._x - _x * r._z, _x * r._y - _y * r._x);
}

Vec3f Vec3f::normalized()
{
	return Vec3f(_x / length(), _y / length(), _z / length());
}

Vec3f Vec3f::rotate(float angle)
{
	return Vec3f();
}

Vec3f Vec3f::lerp(const Vec3f & dest, float lerpFactor)
{
	return Vec3f();
}
