#include "Vec2f.h"



Vec2f::Vec2f()
{
	_x = 0.0f;
	_y = 0.0f;
}


Vec2f::Vec2f(float x, float y)
{
	_x = x;
	_y = y;
}

Vec2f::~Vec2f()
{
}

float Vec2f::getX() const
{
	return _x;
}

float Vec2f::getY() const
{
	return _y;
}

Vec2f Vec2f::operator+(const Vec2f& r)
{
	return Vec2f(_x + r._x, _y + r._y);
}

Vec2f Vec2f::operator+(float r)
{
	return Vec2f(_x + r, _y + r);
}

Vec2f Vec2f::operator-(const Vec2f& r)
{
	return Vec2f(_x - r._x, _y - r._y);
}

Vec2f Vec2f::operator-(float r)
{
	return Vec2f(_x - r, _y - r);
}

Vec2f Vec2f::operator*(const Vec2f& r)
{
	return Vec2f(_x * r._x, _y * r._y);
}

Vec2f Vec2f::operator*(float r)
{
	return Vec2f(_x * r, _y * r);
}

Vec2f Vec2f::operator/(const Vec2f& r)
{
	return Vec2f(_x / r._x, _y / r._y);
}

Vec2f Vec2f::operator/(float r)
{
	return Vec2f(_x / r, _y / r);
}


float Vec2f::max()
{
	if (_x >= _y)
	{
		return _x;
	}
	else { return _y; }
}

float Vec2f::length()
{
	return sqrtf(_x * _x + _y * _y);
}

float Vec2f::dot(const Vec2f& r)
{
	return _x * r._x + _y * r._y;
}

float Vec2f::cross(const Vec2f& r)
{
	return _x * r._y - _y * r._x;
}

Vec2f Vec2f::normalized()
{
	return Vec2f(_x / length(), _y / length());
}

Vec2f Vec2f::rotate(float angle)
{
	return Vec2f();
}

Vec2f Vec2f::lerp(const Vec2f& dest, float lerpFactor)
{
	return Vec2f();
}
