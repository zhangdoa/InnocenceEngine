#pragma once
//
//template <class T> 
//class vec4
//{
//public :
//	vec4();
//	vec4(const vec4<T>& reference);
//	vec4(T x, T y, T z, T w);
//	~vec4();
//
//	void operator=(const vec4<T>& rhs);
//	bool operator==(const vec4<T>& rhs);
//	bool operator!=(const vec4<T>& rhs);
//	vec4<T> operator+(const vec4<T>& rhs);
//	vec4<T> operator-(const vec4<T>& rhs);
//	T dot(const vec4<T>& rhs);
//	vec4<T> cross(const vec4<T>& rhs);
//	T m_x, m_y, m_z, m_w;
//};
//
//template<class T>
//inline vec4<T>::vec4()
//{
//}
//
//template<class T>
//inline vec4<T>::vec4(const vec4<T> & reference)
//{
//
//}
//
//template<class T>
//inline vec4<T>::vec4(T x, T y, T z, T w)
//{
//	m_x = x;
//	m_y = y;
//	m_z = z;
//	m_w = w;
//}
//
//template<class T>
//inline vec4<T>::~vec4()
//{
//}
//
//template<class T>
//inline void vec4<T>::operator=(const vec4<T> & reference)
//{
//	m_x = reference.m_x;
//	m_y = reference.m_y;
//	m_z = reference.m_z;
//	m_w = reference.m_w;
//}
//
//template<class T>
//inline bool vec4<T>::operator==(const vec4<T> & reference)
//{
//	if (m_x != reference.m_x)
//	{
//		return false;
//	}
//	else if(m_y != reference.m_y)
//	{
//		return false;
//	}
//	else if (m_z != reference.m_z)
//	{
//		return false;
//	}
//	else if (m_w != reference.m_w)
//	{
//		return false;
//	}
//	else
//	{
//		return true;
//	}
//}
//
//template<class T>
//inline bool vec4<T>::operator!=(const vec4<T> & reference)
//{
//
//}
//
//template<class T>
//inline vec4<T> vec4<T>::operator+(const vec4<T> & augend)
//{
//	return vec4(m_x + augend.m_x, m_y + augend.m_y, m_z + augend.m_z, m_w + augend.m_w);
//}
//
//template<class T>
//inline vec4<T> vec4<T>::operator-(const vec4<T> & minuend)
//{
//	return vec4(m_x - minuend.m_x, m_y - minuend.m_y, m_z - minuend.m_z, m_w - minuend.m_w);
//}
//
//template<class T>
//inline T vec4<T>::dot(const vec4<T>& multiplicand)
//{
//	return m_x * multiplicand.m_x + m_y * multiplicand.m_y + m_z * multiplicand.m_z + m_w * multiplicand.m_w;
//}
//
//template<class T>
//inline vec4<T> vec4<T>::cross(const vec4<T>& rhs)
//{
//	return vec4<T>(m_y * rhs.m_z - m_z * rhs.m_x, m_z * rhs.m_x - m_x * rhs.m_z, m_x * rhs.m_y - m_y * rhs.m_x, 0);
//}

//typedef __m128 __vec4;
//typedef __vec4 __vec3;
//typedef __vec4 __vec2;

static float sin(float angle);
static float cos(float angle);
static float tan(float angle);
static float cot(float angle);

// [rowIndex][columnIndex]
class mat4
{
public:
	mat4();
	mat4(const mat4& rhs);
	mat4& operator=(const mat4& rhs);
	float* operator[](int index);
	mat4 mul(const mat4& rhs);
	// only for some semantically or literally usage
	mat4 mul(float rhs);

private:
	float m[4][4];
};

//the w component is the last one
class quat
{
public:
	quat();
	quat(float rhsX, float rhsY, float rhsZ, float rhsW);
	quat(const quat& rhs);
	quat& operator=(const quat& rhs);

	~quat();

	quat mul(const quat& rhs);
	// only for some semantically or literally usage
	quat mul(float rhs);
	float length();
	quat normalize();

	mat4 toRotationMartix();

	float x;
	float y;
	float z;
	float w;
};

class vec3
{
public:
	vec3();
	vec3(float rhsX, float rhsY, float rhsZ);
	vec3(const vec3& rhs);
	vec3& operator=(const vec3& rhs);

	~vec3();

	vec3 add(const vec3& rhs);
	vec3 operator+(const vec3& rhs);
	vec3 add(float rhs);
	vec3 operator+(float rhs);
	vec3 sub(const vec3& rhs);
	vec3 operator-(const vec3& rhs);
	vec3 sub(float rhs);
	vec3 operator-(float rhs);
	float dot(const vec3& rhs);
	float dot(float rhs);
	vec3 cross(const vec3& rhs);
	vec3 mul(const vec3& rhs);
	vec3 mul(float rhs);
	float length();
	vec3 normalize();

	mat4 toTranslationMartix();
	mat4 toScaleMartix();

	float x;
	float y;
	float z;
};

class vec2
{
public:
	vec2();
	vec2(float rhsX, float rhsY);
	vec2(const vec2& rhs);
	vec2& operator=(const vec2& rhs);

	~vec2();

	vec2 add(const vec2& rhs);
	vec2 sub(const vec2& rhs);
	float dot(const vec2& rhs);
	float length();
	vec2 normalize();

	float x;
	float y;
};