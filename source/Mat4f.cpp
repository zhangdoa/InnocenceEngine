#include "Mat4f.h"



Mat4f::Mat4f()
{
}


Mat4f::~Mat4f()
{
}

Mat4f Mat4f::initIdentity()
{
	m[0][0] = 1;
	m[0][1] = 0;
	m[0][2] = 0;
	m[0][3] = 0;
	m[1][0] = 0;
	m[1][1] = 1;
	m[1][2] = 0;
	m[1][3] = 0;
	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = 1;
	m[2][3] = 0;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;

	return *this;
}

Mat4f Mat4f::initTranslation(float x, float y, float z)
{
	m[0][0] = 1;
	m[0][1] = 0;
	m[0][2] = 0;
	m[0][3] = x;
	m[1][0] = 0;
	m[1][1] = 1;
	m[1][2] = 0;
	m[1][3] = y;
	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = 1;
	m[2][3] = z;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;

	return *this;
}

Mat4f Mat4f::initRotation(float x, float y, float z)
{
	Mat4f rx;
	Mat4f ry;
	Mat4f rz;

	x = x * PI / 180;
	y = y * PI / 180;
	z = z * PI / 180;

	// set identity matrices
	for (int i = 0; i <= 3; i++) {
		rx.m[i][i] = 1;
		ry.m[i][i] = 1;
		rz.m[i][i] = 1;
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

	return *this;
}

Mat4f Mat4f::initRotation(Vec3f forward, Vec3f up)
{
	return Mat4f();
}

Mat4f Mat4f::initRotation(Vec3f forward, Vec3f up, Vec3f right)
{
	return Mat4f();
}

Mat4f Mat4f::initScale(float x, float y, float z)
{
	m[0][0] = x;
	m[0][1] = 0;
	m[0][2] = 0;
	m[0][3] = 0;
	m[1][0] = 0;
	m[1][1] = y;
	m[1][2] = 0;
	m[1][3] = 0;
	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = z;
	m[2][3] = 0;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 0;
	m[3][3] = 1;

	return *this;
}

Mat4f Mat4f::initPerspective(float fov, float aspectRatio, float zNear, float zFar)
{
	float tanHalfFOV = tanf(fov / 2);
	float zRange = zNear - zFar;

	m[0][0] = 1.0f / (tanHalfFOV * aspectRatio);
	m[0][1] = 0;
	m[0][2] = 0;
	m[0][3] = 0;
	m[1][0] = 0;
	m[1][1] = 1.0f / tanHalfFOV;
	m[1][2] = 0;
	m[1][3] = 0;
	m[2][0] = 0;
	m[2][1] = 0;
	m[2][2] = (-zNear - zFar) / zRange;
	m[2][3] = 2 * zFar * zNear / zRange;
	m[3][0] = 0;
	m[3][1] = 0;
	m[3][2] = 1;
	m[3][3] = 0;

	return *this;
}

Mat4f Mat4f::initOrthographic(float left, float right, float bottom, float top, float near, float)
{
	return Mat4f();
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
