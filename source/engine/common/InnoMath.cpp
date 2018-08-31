#include "InnoMath.h"

template<class T>
//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 vec4::toTranslationMatrix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = 1;
	l_m.m[1][1] = 1;
	l_m.m[2][2] = 1;
	l_m.m[3][0] = x;
	l_m.m[3][1] = y;
	l_m.m[3][2] = z;
	l_m.m[3][3] = 1;

	return l_m;
}
#endif

template<class T>
//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 vec4::toRotationMatrix()
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = (one<T> -two<T> * y * y - two<T> * z * z);
	l_m.m[0][1] = (two<T> * x * y + two<T> * z * w);
	l_m.m[0][2] = (two<T> * x * z - two<T> * y * w);
	l_m.m[0][3] = (T());

	l_m.m[1][0] = (two<T> * x * y - two<T> * z * w);
	l_m.m[1][1] = (one<T> -two<T> * x * x - two<T> * z * z);
	l_m.m[1][2] = (two<T> * y * z + two<T> * x * w);
	l_m.m[1][3] = (T());

	l_m.m[2][0] = (two<T> * x * z + two<T> * y * w);
	l_m.m[2][1] = (two<T> * y * z - two<T> * x * w);
	l_m.m[2][2] = (one<T> -two<T> * x * x - two<T> * y * y);
	l_m.m[2][3] = (T());

	l_m.m[3][0] = (T());
	l_m.m[3][1] = (T());
	l_m.m[3][2] = (T());
	l_m.m[3][3] = (one<T>);

	return l_m;
}
#endif

template<class T>
vec2& vec2::operator=(const vec2 & rhs)
{
	x = rhs.x;
	y = rhs.y;
	return *this;
}

template<class T>
vec2 vec2::operator+(const vec2 & rhs)
{
	return vec2(x + rhs.x, y + rhs.y);
}

template<class T>
vec2 vec2::operator+(double rhs)
{
	return vec2(x + rhs, y + rhs);
}

template<class T>
vec2 vec2::operator-(const vec2 & rhs)
{
	return vec2(x - rhs.x, y - rhs.y);
}

template<class T>
vec2 vec2::operator-(double rhs)
{
	return vec2(x - rhs, y - rhs);
}

template<class T>
T vec2::operator*(const vec2 & rhs)
{
	return x * rhs.x + y * rhs.y;
}

template<class T>
vec2<T> vec2::scale(double rhs)
{
	return vec2(x * rhs, y * rhs);
}

template<class T>
vec2<T> vec2::operator*(double rhs)
{
	return vec2(x * rhs, y * rhs);
}

template<class T>
vec2<T> vec2::operator/(double rhs)
{
	return vec2(x / rhs, y / rhs);
}

template<class T>
mat4 & mat4::operator=(const mat4 & rhs)
{
	m[0][0] = rhs.m[0][0];
	m[0][1] = rhs.m[0][1];
	m[0][2] = rhs.m[0][2];
	m[0][3] = rhs.m[0][3];
	m[1][0] = rhs.m[1][0];
	m[1][1] = rhs.m[1][1];
	m[1][2] = rhs.m[1][2];
	m[1][3] = rhs.m[1][3];
	m[2][0] = rhs.m[2][0];
	m[2][1] = rhs.m[2][1];
	m[2][2] = rhs.m[2][2];
	m[2][3] = rhs.m[2][3];
	m[3][0] = rhs.m[3][0];
	m[3][1] = rhs.m[3][1];
	m[3][2] = rhs.m[3][2];
	m[3][3] = rhs.m[3][3];

	return *this;
}

template<class T>
//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 mat4::operator*(const mat4 & rhs)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = m[0][0] * rhs.m[0][0] + m[1][0] * rhs.m[0][1] + m[2][0] * rhs.m[0][2] + m[3][0] * rhs.m[0][3];
	l_m.m[0][1] = m[0][1] * rhs.m[0][0] + m[1][1] * rhs.m[0][1] + m[2][1] * rhs.m[0][2] + m[3][1] * rhs.m[0][3];
	l_m.m[0][2] = m[0][2] * rhs.m[0][0] + m[1][2] * rhs.m[0][1] + m[2][2] * rhs.m[0][2] + m[3][2] * rhs.m[0][3];
	l_m.m[0][3] = m[0][3] * rhs.m[0][0] + m[1][3] * rhs.m[0][1] + m[2][3] * rhs.m[0][2] + m[3][3] * rhs.m[0][3];

	l_m.m[1][0] = m[0][0] * rhs.m[1][0] + m[1][0] * rhs.m[1][1] + m[2][0] * rhs.m[1][2] + m[3][0] * rhs.m[1][3];
	l_m.m[1][1] = m[0][1] * rhs.m[1][0] + m[1][1] * rhs.m[1][1] + m[2][1] * rhs.m[1][2] + m[3][1] * rhs.m[1][3];
	l_m.m[1][2] = m[0][2] * rhs.m[1][0] + m[1][2] * rhs.m[1][1] + m[2][2] * rhs.m[1][2] + m[3][2] * rhs.m[1][3];
	l_m.m[1][3] = m[0][3] * rhs.m[1][0] + m[1][3] * rhs.m[1][1] + m[2][3] * rhs.m[1][2] + m[3][3] * rhs.m[1][3];

	l_m.m[2][0] = m[0][0] * rhs.m[2][0] + m[1][0] * rhs.m[2][1] + m[2][0] * rhs.m[2][2] + m[3][0] * rhs.m[2][3];
	l_m.m[2][1] = m[0][1] * rhs.m[2][0] + m[1][1] * rhs.m[2][1] + m[2][1] * rhs.m[2][2] + m[3][1] * rhs.m[2][3];
	l_m.m[2][2] = m[0][2] * rhs.m[2][0] + m[1][2] * rhs.m[2][1] + m[2][2] * rhs.m[2][2] + m[3][2] * rhs.m[2][3];
	l_m.m[2][3] = m[0][3] * rhs.m[2][0] + m[1][3] * rhs.m[2][1] + m[2][3] * rhs.m[2][2] + m[3][3] * rhs.m[2][3];

	l_m.m[3][0] = m[0][0] * rhs.m[3][0] + m[1][0] * rhs.m[3][1] + m[2][0] * rhs.m[3][2] + m[3][0] * rhs.m[3][3];
	l_m.m[3][1] = m[0][1] * rhs.m[3][0] + m[1][1] * rhs.m[3][1] + m[2][1] * rhs.m[3][2] + m[3][1] * rhs.m[3][3];
	l_m.m[3][2] = m[0][2] * rhs.m[3][0] + m[1][2] * rhs.m[3][1] + m[2][2] * rhs.m[3][2] + m[3][2] * rhs.m[3][3];
	l_m.m[3][3] = m[0][3] * rhs.m[3][0] + m[1][3] * rhs.m[3][1] + m[2][3] * rhs.m[3][2] + m[3][3] * rhs.m[3][3];

	return l_m;
}
#endif

template<class T>
//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
mat4 mat4::operator*(const mat4 & rhs)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = m[0][0] * rhs.m[0][0] + m[0][1] * rhs.m[1][0] + m[0][2] * rhs.m[2][0] + m[0][3] * rhs.m[3][0];
	l_m.m[0][1] = m[0][0] * rhs.m[0][1] + m[0][1] * rhs.m[1][1] + m[0][2] * rhs.m[2][1] + m[0][3] * rhs.m[3][1];
	l_m.m[0][2] = m[0][0] * rhs.m[0][2] + m[0][1] * rhs.m[1][2] + m[0][2] * rhs.m[2][2] + m[0][3] * rhs.m[3][2];
	l_m.m[0][3] = m[0][0] * rhs.m[0][3] + m[0][1] * rhs.m[1][3] + m[0][2] * rhs.m[2][3] + m[0][3] * rhs.m[3][3];

	l_m.m[1][0] = m[1][0] * rhs.m[0][0] + m[1][1] * rhs.m[1][0] + m[1][2] * rhs.m[2][0] + m[1][3] * rhs.m[3][0];
	l_m.m[1][1] = m[1][0] * rhs.m[0][1] + m[1][1] * rhs.m[1][1] + m[1][2] * rhs.m[2][1] + m[1][3] * rhs.m[3][1];
	l_m.m[1][2] = m[1][0] * rhs.m[0][2] + m[1][1] * rhs.m[1][2] + m[1][2] * rhs.m[2][2] + m[1][3] * rhs.m[3][2];
	l_m.m[1][3] = m[1][0] * rhs.m[0][3] + m[1][1] * rhs.m[1][3] + m[1][2] * rhs.m[2][3] + m[1][3] * rhs.m[3][3];

	l_m.m[2][0] = m[2][0] * rhs.m[0][0] + m[2][1] * rhs.m[1][0] + m[2][2] * rhs.m[2][0] + m[2][3] * rhs.m[3][0];
	l_m.m[2][1] = m[2][0] * rhs.m[0][1] + m[2][1] * rhs.m[1][1] + m[2][2] * rhs.m[2][1] + m[2][3] * rhs.m[3][1];
	l_m.m[2][2] = m[2][0] * rhs.m[0][2] + m[2][1] * rhs.m[1][2] + m[2][2] * rhs.m[2][2] + m[2][3] * rhs.m[3][2];
	l_m.m[2][3] = m[2][0] * rhs.m[0][3] + m[2][1] * rhs.m[1][3] + m[2][2] * rhs.m[2][3] + m[2][3] * rhs.m[3][3];

	l_m.m[3][0] = m[3][0] * rhs.m[0][0] + m[3][1] * rhs.m[1][0] + m[3][2] * rhs.m[2][0] + m[3][3] * rhs.m[3][0];
	l_m.m[3][1] = m[3][0] * rhs.m[0][1] + m[3][1] * rhs.m[1][1] + m[3][2] * rhs.m[2][1] + m[3][3] * rhs.m[3][1];
	l_m.m[3][2] = m[3][0] * rhs.m[0][2] + m[3][1] * rhs.m[1][2] + m[3][2] * rhs.m[2][2] + m[3][3] * rhs.m[3][2];
	l_m.m[3][3] = m[3][0] * rhs.m[0][3] + m[3][1] * rhs.m[1][3] + m[3][2] * rhs.m[2][3] + m[3][3] * rhs.m[3][3];

	return l_m;
}
#endif 

template<class T>
mat4 mat4::operator*(double rhs)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;

	l_m.m[0][0] = rhs * m[0][0];
	l_m.m[0][1] = rhs * m[0][1];
	l_m.m[0][2] = rhs * m[0][2];
	l_m.m[0][3] = rhs * m[0][3];
	l_m.m[1][0] = rhs * m[1][0];
	l_m.m[1][1] = rhs * m[1][1];
	l_m.m[1][2] = rhs * m[1][2];
	l_m.m[1][3] = rhs * m[1][3];
	l_m.m[2][0] = rhs * m[2][0];
	l_m.m[2][1] = rhs * m[2][1];
	l_m.m[2][2] = rhs * m[2][2];
	l_m.m[2][3] = rhs * m[2][3];
	l_m.m[3][0] = rhs * m[3][0];
	l_m.m[3][1] = rhs * m[3][1];
	l_m.m[3][2] = rhs * m[3][2];
	l_m.m[3][3] = rhs * m[3][3];

	return l_m;
}

template<class T>
//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
vec4 mat4::operator*(const vec4 & rhs)
{
	// @TODO: replace with SIMD impl
	vec4 l_vec4;

	l_vec4.x = m[0][0] * rhs.x + m[0][1] * rhs.y + m[0][2] * rhs.z + m[0][3] * rhs.w;
	l_vec4.y = m[1][0] * rhs.x + m[1][1] * rhs.y + m[1][2] * rhs.z + m[1][3] * rhs.w;
	l_vec4.z = m[2][0] * rhs.x + m[2][1] * rhs.y + m[2][2] * rhs.z + m[2][3] * rhs.w;
	l_vec4.w = m[3][0] * rhs.x + m[3][1] * rhs.y + m[3][2] * rhs.z + m[3][3] * rhs.w;

	return l_vec4;
}
#endif

template<class T>
//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double WHRatio, double zNear, double zFar)
{
	m[0][0] = (one<T> / (tan(FOV / two<T>) * WHRatio));
	m[1][1] = (one<T> / tan(FOV / two<T>));
	m[2][2] = (-(zFar + zNear) / ((zFar - zNear)));
	m[2][3] = -one<T>;
	m[3][2] = (-(two<T> * zFar * zNear) / ((zFar - zNear)));
}
#endif

template<class T>
//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToPerspectiveMatrix(double FOV, double WHRatio, double zNear, double zFar)
{
	m[0][0] = (one<T> / (tan(FOV / two<T>) * WHRatio));
	m[1][1] = (one<T> / tan(FOV / two<T>));
	m[2][2] = (-(zFar + zNear) / ((zFar - zNear)));
	m[2][3] = (-(two<T> * zFar * zNear) / ((zFar - zNear)));
	m[3][2] = -one<T>;
}
#endif

template<class T>
//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
void mat4::initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar)
{
	m[0][0] = (two<T> / (right - left));
	m[1][1] = (two<T> / (up - bottom));
	m[2][2] = (-two<T> / (zFar - zNear));
	m[3][0] = (-(right + left) / (right - left));
	m[3][1] = (-(up + bottom) / (up - bottom));
	m[3][2] = (-(zFar + zNear) / (zFar - zNear));
	m[3][3] = one<T>;
}
#endif

template<class T>
//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
void mat4::initializeToOrthographicMatrix(double left, double right, double bottom, double up, double zNear, double zFar)
{
	m[0][0] = (two<T> / (right - left));
	m[0][3] = (-(right + left) / (right - left));
	m[1][1] = (two<T> / (up - bottom));
	m[1][3] = (-(up + bottom) / (up - bottom));
	m[2][2] = (-two<T> / (zFar - zNear));
	m[2][3] = (-(zFar + zNear) / (zFar - zNear));
	m[3][3] = one<T>;
}
#endif

template<class T>
//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
mat4 mat4::lookAt(const vec4 & eyePos, const vec4 & centerPos, const vec4 & upDir)
{
	// @TODO: replace with SIMD impl
	mat4 l_m;
	vec4 l_X;
	vec4 l_Y = upDir;
	vec4 l_Z = vec4(centerPos.x - eyePos.x, centerPos.y - eyePos.y, centerPos.z - eyePos.z, T()).normalize();

	l_X = l_Z.cross(l_Y);
	l_X = l_X.normalize();
	l_Y = l_X.cross(l_Z);
	l_Y = l_Y.normalize();

	l_m.m[0][0] = l_X.x;
	l_m.m[0][1] = l_Y.x;
	l_m.m[0][2] = -l_Z.x;
	l_m.m[0][3] = T();
	l_m.m[1][0] = l_X.y;
	l_m.m[1][1] = l_Y.y;
	l_m.m[1][2] = -l_Z.y;
	l_m.m[1][3] = T();
	l_m.m[2][0] = l_X.z;
	l_m.m[2][1] = l_Y.z;
	l_m.m[2][2] = -l_Z.z;
	l_m.m[2][3] = T();
	l_m.m[3][0] = -(l_X * vec4(eyePos.x, eyePos.y, eyePos.z, T()));
	l_m.m[3][1] = -(l_Y * vec4(eyePos.x, eyePos.y, eyePos.z, T()));
	l_m.m[3][2] = (l_Z * vec4(eyePos.x, eyePos.y, eyePos.z, T()));
	l_m.m[3][3] = one<T>;

	return l_m;
}
#endif

#endif

template<class T>
Vertex& Vertex::operator=(const Vertex & rhs)
{
	m_pos = rhs.m_pos;
	m_texCoord = rhs.m_texCoord;
	m_normal = rhs.m_normal;
	return *this;
}

Ray & Ray::operator=(const Ray & rhs)
{
	m_origin = rhs.m_origin;
	m_direction = rhs.m_direction;

	return *this;
}

AABB & AABB::operator=(const AABB & rhs)
{
	m_center = rhs.m_center;
	m_sphereRadius = rhs.m_sphereRadius;
	m_boundMin = rhs.m_boundMin;
	m_boundMax = rhs.m_boundMax;
	m_vertices = rhs.m_vertices;
	m_indices = rhs.m_indices;
	return *this;
}

bool AABB::intersectCheck(const AABB & rhs)
{
	if (rhs.m_center.x - m_center.x > rhs.m_sphereRadius + m_sphereRadius) return false;
	if (rhs.m_center.y - m_center.y > rhs.m_sphereRadius + m_sphereRadius) return false;
	if (rhs.m_center.z - m_center.z > rhs.m_sphereRadius + m_sphereRadius) return false;
	else return true;
}

bool AABB::intersectCheck(const Ray & rhs)
{
	double txmin, txmax, tymin, tymax, tzmin, tzmax;
	double l_invDirectionX = std::isinf(1.0 / rhs.m_direction.x) ? 0.0 : 1.0 / rhs.m_direction.x;
	double l_invDirectionY = std::isinf(1.0 / rhs.m_direction.y) ? 0.0 : 1.0 / rhs.m_direction.y;
	double l_invDirectionZ = std::isinf(1.0 / rhs.m_direction.z) ? 0.0 : 1.0 / rhs.m_direction.z;
	vec4<double> l_invDirection = vec4<double>(l_invDirectionX, l_invDirectionY, l_invDirectionZ, 0.0).normalize();

	if (l_invDirection.x >= 0.0) {
		txmin = (m_boundMin.x - rhs.m_origin.x) * l_invDirection.x;
		txmax = (m_boundMax.x - rhs.m_origin.x) * l_invDirection.x;
	}
	else {
		txmin = (m_boundMax.x - rhs.m_origin.x) * l_invDirection.x;
		txmax = (m_boundMin.x - rhs.m_origin.x) * l_invDirection.x;
	}
	if (l_invDirection.y >= 0.0) {
		tymin = (m_boundMin.y - rhs.m_origin.y) * l_invDirection.y;
		tymax = (m_boundMax.y - rhs.m_origin.y) * l_invDirection.y;
	}
	else {
		tymin = (m_boundMax.y - rhs.m_origin.y) * l_invDirection.y;
		tymax = (m_boundMin.y - rhs.m_origin.y) * l_invDirection.y;
	}
	if (txmin > tymax || tymin > txmax)
		return false;
	if (l_invDirection.z >= 0.0) {
		tzmin = (m_boundMin.z - rhs.m_origin.z) * l_invDirection.z;
		tzmax = (m_boundMax.z - rhs.m_origin.z) * l_invDirection.z;
	}
	else {
		tzmin = (m_boundMax.z - rhs.m_origin.z) * l_invDirection.z;
		tzmax = (m_boundMin.z - rhs.m_origin.z) * l_invDirection.z;
	}
	if (txmin > tzmax || tzmin > txmax)
		return false;
	txmin = (tymin > txmin) || std::isinf(txmin) ? tymin : txmin;
	txmax = (tymax < txmax) || std::isinf(txmax) ? tymax : txmax;
	txmin = (tzmin > txmin) ? tzmin : txmin;
	txmax = (tzmax < txmax) ? tzmax : txmax;
	if (txmin < txmax && txmax >= 0.0) {
		return true;
	}
	return false;
}

bool Transform::hasChanged()
{
	if (m_pos != m_previousPos || m_rot != m_previousRot || m_scale != m_previousScale)
	{
		return true;
	}

	if (nullptr != m_parentTransform)
	{
		if (m_parentTransform->hasChanged())
		{
			return true;
		}
	}
	return false;

}

void Transform::update()
{
	m_previousPos = m_pos;
	m_previousRot = m_rot;
	m_previousScale = m_scale;
}

void Transform::rotateInLocal(const vec4<double> & axis, double angle)
{
	double sinHalfAngle = sin((angle * PI / 180) / 2.0);
	double cosHalfAngle = cos((angle * PI / 180) / 2.0);
	// get final rotation
	m_rot = vec4<double>(axis.x * sinHalfAngle, axis.y * sinHalfAngle, axis.z * sinHalfAngle, cosHalfAngle).quatMul(m_rot);
}

vec4<double> & Transform::getPos()
{
	return m_pos;
}

vec4<double> & Transform::getRot()
{
	return m_rot;
}

vec4<double> & Transform::getScale()
{
	return m_scale;
}

void Transform::setLocalPos(const vec4<double> & pos)
{
	m_pos = pos;
}

void Transform::setLocalRot(const vec4<double> & rot)
{
	m_rot = rot;
}

void Transform::setLocalScale(const vec4<double> & scale)
{
	m_scale = scale;
}

mat4<double> Transform::caclLocalTranslationMatrix()
{
	return m_pos.toTranslationMatrix();
}

mat4<double> Transform::caclLocalRotMatrix()
{
	return m_rot.toRotationMatrix();
}

mat4<double> Transform::caclLocalScaleMatrix()
{
	return m_scale.toScaleMatrix();
}

mat4<double> Transform::caclLocalTransformationMatrix()
{
	return caclLocalTranslationMatrix() * caclLocalRotMatrix() * caclLocalScaleMatrix();
}

mat4<double> Transform::caclPreviousLocalTranslationMatrix()
{
	return m_previousPos.toTranslationMatrix();
}

mat4<double> Transform::caclPreviousLocalRotMatrix()
{
	return m_previousRot.toRotationMatrix();
}

mat4<double> Transform::caclPreviousLocalScaleMatrix()
{
	return m_previousScale.toScaleMatrix();
}

mat4<double> Transform::caclPreviousLocalTransformationMatrix()
{
	return caclPreviousLocalTranslationMatrix() * caclPreviousLocalRotMatrix() * caclPreviousLocalScaleMatrix();
}

vec4<double> Transform::caclGlobalPos()
{
	mat4<double> l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclGlobalTransformationMatrix();
	}

	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	auto result = vec4();
	result = m_pos * l_parentTransformationMatrix;
	result = result * (1 / result.w);
	return result;
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	auto result = vec4<double>();
	result = l_parentTransformationMatrix * m_pos;
	result = result * (1 / result.w);
	return result;
#endif
}

vec4<double> Transform::caclGlobalRot()
{
	vec4<double> l_parentRot = vec4<double>(0.0, 0.0, 0.0, 1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentRot = m_parentTransform->caclGlobalRot();
	}

	return l_parentRot.quatMul(m_rot);
}

vec4<double> Transform::caclGlobalScale()
{
	vec4<double> l_parentScale = vec4<double>(1.0,1.0,1.0,1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentScale = m_parentTransform->caclGlobalScale();
	}

	return l_parentScale.scale(m_scale);
}

vec4<double> Transform::caclPreviousGlobalPos()
{
	mat4<double> l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclPreviousGlobalTransformationMatrix();
	}

	//Column-Major memory layout
#ifdef USE_COLUMN_MAJOR_MEMORY_LAYOUT
	auto result = vec4();
	result = m_previousPos * l_parentTransformationMatrix;
	result = result * (1 / result.w);
	return result;
#endif
	//Row-Major memory layout
#ifdef USE_ROW_MAJOR_MEMORY_LAYOUT
	auto result = vec4<double>();
	result = l_parentTransformationMatrix * m_previousPos;
	result = result * (1.0 / result.w);
	return result;
#endif
}

vec4<double> Transform::caclPreviousGlobalRot()
{
	vec4<double> l_parentRot = vec4<double>(0.0,0.0,0.0,1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentRot = m_parentTransform->caclPreviousGlobalRot();
	}

	return l_parentRot.quatMul(m_previousRot);
}

vec4<double> Transform::caclPreviousGlobalScale()
{
	vec4<double> l_parentScale = vec4<double>(1.0, 1.0, 1.0, 1.0);

	if (nullptr != m_parentTransform)
	{
		l_parentScale = m_parentTransform->caclPreviousGlobalScale();
	}

	return l_parentScale.scale(m_previousScale);
}

mat4<double> Transform::caclGlobalTranslationMatrix()
{
	return caclGlobalPos().toTranslationMatrix();
}

mat4<double> Transform::caclGlobalRotMatrix()
{
	return caclGlobalRot().toRotationMatrix();
}

mat4<double> Transform::caclGlobalScaleMatrix()
{
	return caclGlobalScale().toScaleMatrix();
}

mat4<double> Transform::caclGlobalTransformationMatrix()
{
	mat4<double> l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclGlobalTransformationMatrix();
	}

	return l_parentTransformationMatrix * caclLocalTransformationMatrix();
}

mat4<double> Transform::caclPreviousGlobalTranslationMatrix()
{
	return caclPreviousGlobalPos().toTranslationMatrix();
}

mat4<double> Transform::caclPreviousGlobalRotMatrix()
{
	return caclPreviousGlobalRot().toRotationMatrix();
}

mat4<double> Transform::caclPreviousGlobalScaleMatrix()
{
	return caclPreviousGlobalScale().toScaleMatrix();
}

mat4<double> Transform::caclPreviousGlobalTransformationMatrix()
{
	mat4<double> l_parentTransformationMatrix;
	l_parentTransformationMatrix.initializeToIdentityMatrix();

	if (nullptr != m_parentTransform)
	{
		l_parentTransformationMatrix = m_parentTransform->caclPreviousGlobalTransformationMatrix();
	}

	return l_parentTransformationMatrix * caclPreviousLocalTransformationMatrix();

}

mat4<double> Transform::caclLookAtMatrix()
{
	return mat4<double>().lookAt(caclGlobalPos(), caclGlobalPos() + getDirection(direction::BACKWARD), getDirection(direction::UP));
}

mat4<double> Transform::caclPreviousLookAtMatrix()
{
	return mat4<double>().lookAt(caclPreviousGlobalPos(), caclPreviousGlobalPos() + getPreviousDirection(direction::BACKWARD), getPreviousDirection(direction::UP));
}

mat4<double> Transform::getInvertLocalTranslationMatrix()
{
	return m_pos.scale(-1.0).toTranslationMatrix();
}

mat4<double> Transform::getInvertLocalRotMatrix()
{
	return m_rot.quatConjugate().toRotationMatrix();
}

mat4<double> Transform::getInvertLocalScaleMatrix()
{
	return m_scale.reciprocal().toScaleMatrix();
}

mat4<double> Transform::getInvertGlobalTranslationMatrix()
{
	return caclGlobalPos().scale(-1.0).toTranslationMatrix();
}

mat4<double> Transform::getInvertGlobalRotMatrix()
{
	return caclGlobalRot().quatConjugate().toRotationMatrix();
}

mat4<double> Transform::getInvertGlobalScaleMatrix()
{
	return caclGlobalScale().reciprocal().toScaleMatrix();
}

mat4<double> Transform::getPreviousInvertLocalTranslationMatrix()
{
	return m_previousPos.scale(-1.0).toTranslationMatrix();
}

mat4<double> Transform::getPreviousInvertLocalRotMatrix()
{
	return m_previousRot.quatConjugate().toRotationMatrix();
}

mat4<double> Transform::getPreviousInvertLocalScaleMatrix()
{
	return m_previousScale.reciprocal().toScaleMatrix();
}

mat4<double> Transform::getPreviousInvertGlobalTranslationMatrix()
{
	return caclPreviousGlobalPos().scale(-1.0).toTranslationMatrix();
}

mat4<double> Transform::getPreviousInvertGlobalRotMatrix()
{
	return caclPreviousGlobalRot().quatConjugate().toRotationMatrix();
}

mat4<double> Transform::getPreviousInvertGlobalScaleMatrix()
{
	return caclPreviousGlobalScale().reciprocal().toScaleMatrix();
}

vec4<double> Transform::getDirection(direction direction) const
{
	vec4<double> l_directionVec4;

	switch (direction)
	{
	case FORWARD: l_directionVec4 = vec4<double>(0.0f, 0.0f, 1.0f, 0.0f); break;
	case BACKWARD:l_directionVec4 = vec4<double>(0.0f, 0.0f, -1.0f, 0.0f); break;
	case UP:l_directionVec4 = vec4<double>(0.0f, 1.0f, 0.0f, 0.0f); break;
	case DOWN:l_directionVec4 = vec4<double>(0.0f, -1.0f, 0.0f, 0.0f); break;
	case RIGHT:l_directionVec4 = vec4<double>(1.0f, 0.0f, 0.0f, 0.0f); break;
	case LEFT:l_directionVec4 = vec4<double>(-1.0f, 0.0f, 0.0f, 0.0f); break;
	}

	// V' = QVQ^-1, for unit quaternion, the conjugated quaternion is as same as the inverse quaternion

	// naive version
	// get Q * V by hand
	//vec4 l_hiddenRotatedQuat;
	//l_hiddenRotatedQuat.w = -m_rot.x * l_directionvec4.x - m_rot.y * l_directionvec4.y - m_rot.z * l_directionvec4.z;
	//l_hiddenRotatedQuat.x = m_rot.w * l_directionvec4.x + m_rot.y * l_directionvec4.z - m_rot.z * l_directionvec4.y;
	//l_hiddenRotatedQuat.y = m_rot.w * l_directionvec4.y + m_rot.z * l_directionvec4.x - m_rot.x * l_directionvec4.z;
	//l_hiddenRotatedQuat.z = m_rot.w * l_directionvec4.z + m_rot.x * l_directionvec4.y - m_rot.y * l_directionvec4.x;

	// get conjugated quaternion
	//vec4 l_conjugatedQuat;
	//l_conjugatedQuat = conjugate(m_rot);

	// then QV * Q^-1 
	//vec4 l_directionQuat;
	//l_directionQuat = l_hiddenRotatedQuat * l_conjugatedQuat;
	//l_directionvec4.x = l_directionQuat.x;
	//l_directionvec4.y = l_directionQuat.y;
	//l_directionvec4.z = l_directionQuat.z;

	// traditional version, change direction vector to quaternion representation

	//vec4 l_directionQuat = vec4(0.0, l_directionvec4);
	//l_directionQuat = m_rot * l_directionQuat * conjugate(m_rot);
	//l_directionvec4.x = l_directionQuat.x;
	//l_directionvec4.y = l_directionQuat.y;
	//l_directionvec4.z = l_directionQuat.z;

	// optimized version ([Kavan et al. ] Lemma 4)
	//V' = V + 2 * Qv x (Qv x V + Qs * V)
	vec4<double> l_Qv = vec4<double>(m_rot.x, m_rot.y, m_rot.z, m_rot.w);
	l_directionVec4 = l_directionVec4 + l_Qv.cross((l_Qv.cross(l_directionVec4) + l_directionVec4.scale(m_rot.w))).scale(2.0f);

	return l_directionVec4.normalize();
}

vec4<double> Transform::getPreviousDirection(direction direction) const
{
	vec4<double> l_directionVec4;

	switch (direction)
	{
	case FORWARD: l_directionVec4 = vec4<double>(0.0f, 0.0f, 1.0f, 0.0f); break;
	case BACKWARD:l_directionVec4 = vec4<double>(0.0f, 0.0f, -1.0f, 0.0f); break;
	case UP:l_directionVec4 = vec4<double>(0.0f, 1.0f, 0.0f, 0.0f); break;
	case DOWN:l_directionVec4 = vec4<double>(0.0f, -1.0f, 0.0f, 0.0f); break;
	case RIGHT:l_directionVec4 = vec4<double>(1.0f, 0.0f, 0.0f, 0.0f); break;
	case LEFT:l_directionVec4 = vec4<double>(-1.0f, 0.0f, 0.0f, 0.0f); break;
	}

	// V' = QVQ^-1, for unit quaternion, the conjugated quaternion is as same as the inverse quaternion

	// naive version
	// get Q * V by hand
	//vec4 l_hiddenRotatedQuat;
	//l_hiddenRotatedQuat.w = -m_rot.x * l_directionvec4.x - m_rot.y * l_directionvec4.y - m_rot.z * l_directionvec4.z;
	//l_hiddenRotatedQuat.x = m_rot.w * l_directionvec4.x + m_rot.y * l_directionvec4.z - m_rot.z * l_directionvec4.y;
	//l_hiddenRotatedQuat.y = m_rot.w * l_directionvec4.y + m_rot.z * l_directionvec4.x - m_rot.x * l_directionvec4.z;
	//l_hiddenRotatedQuat.z = m_rot.w * l_directionvec4.z + m_rot.x * l_directionvec4.y - m_rot.y * l_directionvec4.x;

	// get conjugated quaternion
	//vec4 l_conjugatedQuat;
	//l_conjugatedQuat = conjugate(m_rot);

	// then QV * Q^-1 
	//vec4 l_directionQuat;
	//l_directionQuat = l_hiddenRotatedQuat * l_conjugatedQuat;
	//l_directionvec4.x = l_directionQuat.x;
	//l_directionvec4.y = l_directionQuat.y;
	//l_directionvec4.z = l_directionQuat.z;

	// traditional version, change direction vector to quaternion representation

	//vec4 l_directionQuat = vec4(0.0, l_directionvec4);
	//l_directionQuat = m_rot * l_directionQuat * conjugate(m_rot);
	//l_directionvec4.x = l_directionQuat.x;
	//l_directionvec4.y = l_directionQuat.y;
	//l_directionvec4.z = l_directionQuat.z;

	// optimized version ([Kavan et al. ] Lemma 4)
	//V' = V + 2 * Qv x (Qv x V + Qs * V)
	vec4<double> l_Qv = vec4<double>(m_previousRot.x, m_previousRot.y, m_previousRot.z, m_previousRot.w);
	l_directionVec4 = l_directionVec4 + l_Qv.cross((l_Qv.cross(l_directionVec4) + l_directionVec4.scale(m_previousRot.w))).scale(2.0f);

	return l_directionVec4.normalize();

}