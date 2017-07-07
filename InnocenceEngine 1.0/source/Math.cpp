#include "stdafx.h"
#include "Math.h"


Math::Math()
{
}


Math::~Math()
{
}

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

Vec2f Vec2f::operator+(const Vec2f& r) const
{
	return Vec2f(_x + r._x, _y + r._y);
}

Vec2f Vec2f::operator+(float r) const
{
	return Vec2f(_x + r, _y + r);
}

Vec2f Vec2f::operator-(const Vec2f& r) const
{
	return Vec2f(_x - r._x, _y - r._y);
}

Vec2f Vec2f::operator-(float r) const
{
	return Vec2f(_x - r, _y - r);
}

Vec2f Vec2f::operator*(const Vec2f& r) const
{
	return Vec2f(_x * r._x, _y * r._y);
}

Vec2f Vec2f::operator*(float r) const
{
	return Vec2f(_x * r, _y * r);
}

Vec2f Vec2f::operator/(const Vec2f& r) const
{
	return Vec2f(_x / r._x, _y / r._y);
}

Vec2f Vec2f::operator/(float r) const
{
	return Vec2f(_x / r, _y / r);
}

bool Vec2f::operator!=(const Vec2f & r) const
{
	if (_x != r._x || _y != r._y)
	{
		return true;
	}
	else
	{
		return false;
	}
}


float Vec2f::getMaxElem() const
{
	if (_x >= _y)
	{
		return _x;
	}
	else { return _y; }
}

float Vec2f::getLength() const
{
	return sqrtf(_x * _x + _y * _y);
}

float Vec2f::dot(const Vec2f& r) const
{
	return _x * r._x + _y * r._y;
}

float Vec2f::cross(const Vec2f& r) const
{
	return _x * r._y - _y * r._x;
}

void Vec2f::normalize()
{
	_x = _x / getLength();
	_y = _y / getLength();
}

Vec2f Vec2f::getNormalizedVec2f() const
{
	return Vec2f(_x / getLength(), _y / getLength());
}

void Vec2f::rotate(float angle)
{

}

Vec2f Vec2f::getRotatedVec2f(float angle) const
{
	return Vec2f();
}

Vec2f Vec2f::lerp(const Vec2f& dest, float lerpFactor) const
{
	return Vec2f();
}


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

Vec3f Vec3f::operator+(const Vec3f& r) const
{
	return Vec3f(_x + r._x, _y + r._y, _z + r._z);
}

Vec3f Vec3f::operator+(float r) const
{
	return Vec3f(_x + r, _y + r, _z + r);
}

Vec3f Vec3f::operator-(const Vec3f& r) const
{
	return Vec3f(_x - r._x, _y - r._y, _z - r._z);
}

Vec3f Vec3f::operator-(float r) const
{
	return Vec3f(_x - r, _y - r, _z - r);
}

Vec3f Vec3f::operator*(const Vec3f& r) const
{
	return Vec3f(_x * r._x, _y * r._y, _z * r._z);
}

Vec3f Vec3f::operator*(float r) const
{
	return Vec3f(_x * r, _y * r, _z * r);
}

Vec3f Vec3f::operator/(const Vec3f& r) const
{
	return Vec3f(_x / r._x, _y / r._y, _z / r._z);
}

Vec3f Vec3f::operator/(float r) const
{
	return Vec3f(_x * r, _y * r, _z * r);
}

bool Vec3f::operator!=(const Vec3f & r) const
{
	if (_x != r._x || _y != r._y || _z != r._z)
	{
		return true;
	}
	else {
		return false;
	}
}

float Vec3f::getMaxElem() const
{
	if (_x >= _y && _x >= _z)
	{
		return _x;
	}
	else if (_y >= _x && _y >= _z)
	{
		return _y;
	}
	return _z;
}

float Vec3f::getLength() const
{
	return sqrtf(_x * _x + _y * _y + _z * _z);
}

float Vec3f::dot(const Vec3f& r) const
{
	return _x * r._x + _y * r._y + _z * r._z;
}

Vec3f Vec3f::cross(const Vec3f& r) const
{
	return Vec3f(_y * r._z - _z * r._y, _z * r._x - _x * r._z, _x * r._y - _y * r._x);
}

void Vec3f::normalize()
{
	_x = _x / getLength();
	_y = _y / getLength();
	_z = _z / getLength();
}

Vec3f Vec3f::getNormalizedVec3f() const
{
	return Vec3f(_x / getLength(), _y / getLength(), _z / getLength());
}

void Vec3f::rotate(const Vec3f& axis, float angle)
{

}

void Vec3f::rotate(const Vec4f& rotation)
{

}

Vec3f Vec3f::getRotatedVec3f(const Vec3f& axis, float angle) const
{
	return Vec3f();
}

Vec3f Vec3f::getRotatedVec3f(const Vec4f & rotation) const
{
	Vec4f w = (rotation * (*this)) *(rotation.getConjugatedVec4f());
	return Vec3f(w.getX(), w.getY(), w.getZ());
}

Vec3f Vec3f::lerp(const Vec3f& dest, float lerpFactor) const
{
	return Vec3f();
}


Mat4f::Mat4f()
{
}


Mat4f::~Mat4f()
{
}

void Mat4f::initIdentity()
{

	m[0][0] = 1.0f;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = 1.0f;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 1.0f;
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;

}

void Mat4f::initTranslation(float x, float y, float z)
{
	m[0][0] = 1.0f;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = x;
	m[1][0] = 0.0f;
	m[1][1] = 1.0f;
	m[1][2] = 0.0f;
	m[1][3] = y;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = 1.0f;
	m[2][3] = z;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

void Mat4f::initRotation(float x, float y, float z)
{
	Mat4f rx;
	Mat4f ry;
	Mat4f rz;

	x = x * PI / 180.0f;
	y = y * PI / 180.0f;
	z = z * PI / 180.0f;

	// set identity matrices
	for (int i = 0; i <= 3; i++) {
		rx.m[i][i] = 1.0f;
		ry.m[i][i] = 1.0f;
		rz.m[i][i] = 1.0f;
	}

	// set rotation matrix for x component
	rx.m[1][1] = cos(x);
	rx.m[1][2] = -sin(x);
	rx.m[2][1] = sin(x);
	rx.m[2][2] = cos(x);

	// set rotation matrix for y component
	ry.m[0][0] = cos(y);
	ry.m[0][2] = -sin(y);
	ry.m[2][0] = sin(y);
	ry.m[2][2] = cos(y);

	// set rotation matrix for z component
	rz.m[0][0] = cos(z);
	rz.m[0][1] = -sin(z);
	rz.m[1][0] = sin(z);
	rz.m[1][1] = cos(z);

	this->setAllElem((rz * ry * rx).m);
}

void Mat4f::initRotation(Vec3f forward, Vec3f up)
{
}

void Mat4f::initRotation(Vec3f forward, Vec3f up, Vec3f right)
{
	m[0][0] = right.getX();
	m[0][1] = right.getY();
	m[0][2] = right.getZ();
	m[0][3] = 0;
	m[1][0] = up.getX();
	m[1][1] = up.getY();
	m[1][2] = up.getZ();
	m[1][3] = 0;
	m[2][0] = forward.getX();
	m[2][1] = forward.getY();
	m[2][2] = forward.getZ();
	m[2][3] = 0;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;

}

void Mat4f::initScale(float x, float y, float z)
{
	m[0][0] = x;
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = y;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = z;
	m[2][3] = 0.0f;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 1.0f;
}

void Mat4f::initPerspective(float fov, float aspectRatio, float zNear, float zFar)
{
	float tanHalfFOV = tanf(fov / 2);
	float zRange = zNear - zFar;

	m[0][0] = 1.0f / (tanHalfFOV * aspectRatio);
	m[0][1] = 0.0f;
	m[0][2] = 0.0f;
	m[0][3] = 0.0f;
	m[1][0] = 0.0f;
	m[1][1] = 1.0f / tanHalfFOV;
	m[1][2] = 0.0f;
	m[1][3] = 0.0f;
	m[2][0] = 0.0f;
	m[2][1] = 0.0f;
	m[2][2] = (-zNear - zFar) / zRange;
	m[2][3] = 2 * zFar * zNear / zRange;
	m[3][0] = 0.0f;
	m[3][1] = 0.0f;
	m[3][2] = 0.0f;
	m[3][3] = 0.0f;
}

void Mat4f::initOrthographic(float left, float right, float bottom, float top, float near, float)
{
}

Mat4f Mat4f::operator*(const Mat4f & r)
{
	Mat4f res;

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			res.setElem(i, j, m[i][0] * r.m[0][j] + m[i][1] * r.m[1][j] + m[i][2] * r.m[2][j] + m[i][3] * r.m[3][j]);
		}
	}

	return res;
}

Vec3f Mat4f::transform(Vec3f r)
{
	return Vec3f(m[0][0] * r.getX() + m[0][1] * r.getY() + m[0][2] * r.getZ() + m[0][3],
		m[1][0] * r.getX() + m[1][1] * r.getY() + m[1][2] * r.getZ() + m[1][3],
		m[2][0] * r.getX() + m[2][1] * r.getY() + m[2][2] * r.getZ() + m[2][3]);
}

float Mat4f::getElem(int x, int y) const
{
	return m[x][y];
}

void Mat4f::setElem(int x, int y, float value)
{
	m[x][y] = value;
}

float* Mat4f::getAllElem()
{
	return *m;
}

void Mat4f::setAllElem(float r[4][4])
{
	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			m[i][j] = r[i][j];
		}
	}
}

Vec4f::Vec4f()
{
	_x = 0.0f;
	_y = 0.0f;
	_z = 0.0f;
	_w = 0.0f;
}

Vec4f::Vec4f(float x, float y, float z, float w)
{
	_x = x;
	_y = y;
	_z = z;
	_w = w;

}

Vec4f::Vec4f(const Vec3f& axis, float angle)
{
	float sinHalfAngle = sinf(angle / 2);
	float cosHalfAngle = cosf(angle / 2);

	_x = axis.getX() * sinHalfAngle;
	_y = axis.getY() * sinHalfAngle;
	_z = axis.getZ() * sinHalfAngle;
	_w = cosHalfAngle;

}

Vec4f::~Vec4f()
{
}

float Vec4f::getX() const
{
	return _x;
}

float Vec4f::getY() const
{
	return _y;
}

float Vec4f::getZ() const
{
	return _z;
}

float Vec4f::getW() const
{
	return _w;
}


Vec4f Vec4f::operator*(const Vec4f& r) const
{
	float w_ = _w * r.getW() - _x * r.getX() - _y * r.getY() - _z * r.getZ();
	float x_ = _x * r.getW() + _w * r.getX() + _y * r.getZ() - _z * r.getY();
	float y_ = _y * r.getW() + _w * r.getY() + _z * r.getX() - _x * r.getZ();
	float z_ = _z * r.getW() + _w * r.getZ() + _x * r.getY() - _y * r.getX();

	return Vec4f(x_, y_, z_, w_);
}

Vec4f Vec4f::operator*(const Vec3f& r) const
{
	float w_ = -_x * r.getX() - _y * r.getY() - _z * r.getZ();
	float x_ = _w * r.getX() + _y * r.getZ() - _z * r.getY();
	float y_ = _w * r.getY() + _z * r.getX() - _x * r.getZ();
	float z_ = _w * r.getZ() + _x * r.getY() - _y * r.getX();

	return Vec4f(x_, y_, z_, w_);
}

Vec4f Vec4f::operator*(float r) const
{
	return Vec4f(_x * r, _y * r, _z * r, _w * r);
}

bool Vec4f::operator!=(const Vec4f & r) const
{
	if (_x != r.getX() || _y != r.getY() || _z != r.getZ() || _w != r.getW())
	{
		return true;
	}
	else {
		return false;
	}
}


float Vec4f::getMaxElem() const
{
	return 0.0f;
}

float Vec4f::getLength() const
{
	return sqrtf(_x * _x + _y * _y + _z * _z + _w * _w);
}

float Vec4f::dot(const Vec4f & r) const
{
	return 0.0f;
}

Vec4f Vec4f::cross(const Vec4f & r) const
{
	return Vec4f();
}

void Vec4f::normalize()
{
	_x = _x / getLength();
	_y = _y / getLength();
	_z = _z / getLength();
	_w = _w / getLength();
}

Vec4f Vec4f::getNormalizedVec4f() const
{
	return Vec4f(_x / getLength(), _y / getLength(), _z / getLength(), _w / getLength());
}

void Vec4f::rotate(float angle)
{
}

Vec4f Vec4f::getRotatedVec4f(float angle) const
{
	return Vec4f();
}

Vec4f Vec4f::getRotatedVec4f(const Vec4f & rotation) const
{
	return Vec4f();
}

Vec4f Vec4f::lerp(const Vec4f & dest, float lerpFactor) const
{
	return Vec4f();
}

void Vec4f::conjugate()
{

	_x = -_x;
	_y = -_y;
	_z = -_z;
	_w = -_w;
}

Vec4f Vec4f::getConjugatedVec4f() const
{
	return Vec4f(-_x, -_y, -_z, -_w);
}

Mat4f Vec4f::toRotationMatrix() const
{
	Vec3f forward = Vec3f(2.0f * (_x * _z - _w * _y), 2.0f * (_y * _z + _w * _x), 1.0f - 2.0f * (_x * _x + _y * _y));
	Vec3f up = Vec3f(2.0f * (_x * _y + _w * _z), 1.0f - 2.0f * (_x * _x + _z * _z), 2.0f * (_y * _z - _w * _x));
	Vec3f right = Vec3f(1.0f - 2.0f * (_y * _y + _z * _z), 2.0f * (_x * _y - _w * _z), 2.0f * (_x * _z + _w * _y));
	Mat4f rotationMatrix;
	rotationMatrix.initRotation(forward, up, right);
	return rotationMatrix;
}

Vec3f Vec4f::getForward() const
{
	return Vec3f(0, 0, 1).getRotatedVec3f(*this);
}

Vec3f Vec4f::getBackward() const
{
	return Vec3f(0, 0, -1).getRotatedVec3f(*this);
}

Vec3f Vec4f::getUp() const
{
	return Vec3f(0, 1, 0).getRotatedVec3f(*this);
}

Vec3f Vec4f::getDown() const
{
	return Vec3f(0, -1, 0).getRotatedVec3f(*this);
}

Vec3f Vec4f::getRight() const
{
	return Vec3f(1, 0, 0).getRotatedVec3f(*this);
}

Vec3f Vec4f::getLeft() const
{
	return Vec3f(-1, 0, 0).getRotatedVec3f(*this);
}


Transform::Transform()
{
	_pos = Vec3f(0.0f, 0.0f, 0.0f);
	_rot = Vec4f(0.0f, 0.0f, 0.0f, 1.0f);
	_scale = Vec3f(1.0f, 1.0f, 1.0f);
}

Transform::~Transform()
{
}

void Transform::update()
{

	_oldPos = _pos;
	_oldRot = _rot;
	_oldScale = _scale;
	/*_oldPos = _pos + (1.0f);
	_oldRot = _rot *(0.5f);
	_oldScale = _scale + (1.0f);*/
}

void Transform::rotate(Vec3f axis, float angle)
{
	_rot = (Vec4f(axis, angle) * _rot).getNormalizedVec4f();
}

const Vec3f & Transform::getPos()
{
	return _pos;
}

const Vec4f & Transform::getRot()
{
	return _rot;
}

const Vec3f & Transform::getScale()
{
	return _scale;
}

void Transform::setPos(const Vec3f & pos)
{
	_pos = pos;
}

void Transform::setRot(const Vec4f & rot)
{
	_rot = rot;
}

void Transform::setScale(const Vec3f & scale)
{
	_scale = scale;
}

const Vec3f & Transform::getOldPos()
{
	return _oldPos;
}

const Vec4f & Transform::getOldRot()
{
	return _oldRot;
}

const Vec3f & Transform::getOldScale()
{
	return _oldScale;
}