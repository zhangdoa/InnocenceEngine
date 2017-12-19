#pragma once

template <class T> 
class vec4
{
public :
	vec4();
	vec4(const vec4<T>& reference);
	vec4(T x, T y, T z, T w);
	~vec4();

	void operator=(const vec4<T>& rhs);
	bool operator==(const vec4<T>& rhs);
	bool operator!=(const vec4<T>& rhs);
	vec4<T> operator+(const vec4<T>& rhs);
	vec4<T> operator-(const vec4<T>& rhs);
	T dot(const vec4<T>& rhs);
	vec4<T> cross(const vec4<T>& rhs);
	T m_x, m_y, m_z, m_w;
};

template<class T>
inline vec4<T>::vec4()
{
}

template<class T>
inline vec4<T>::vec4(const vec4<T> & reference)
{

}

template<class T>
inline vec4<T>::vec4(T x, T y, T z, T w)
{
	m_x = x;
	m_y = y;
	m_z = z;
	m_w = w;
}

template<class T>
inline vec4<T>::~vec4()
{
}

template<class T>
inline void vec4<T>::operator=(const vec4<T> & reference)
{
	m_x = reference.m_x;
	m_y = reference.m_y;
	m_z = reference.m_z;
	m_w = reference.m_w;
}

template<class T>
inline bool vec4<T>::operator==(const vec4<T> & reference)
{
	if (m_x != reference.m_x)
	{
		return false;
	}
	else if(m_y != reference.m_y)
	{
		return false;
	}
	else if (m_z != reference.m_z)
	{
		return false;
	}
	else if (m_w != reference.m_w)
	{
		return false;
	}
	else
	{
		return true;
	}
}

template<class T>
inline bool vec4<T>::operator!=(const vec4<T> & reference)
{

}

template<class T>
inline vec4<T> vec4<T>::operator+(const vec4<T> & augend)
{
	return vec4(m_x + augend.m_x, m_y + augend.m_y, m_z + augend.m_z, m_w + augend.m_w);
}

template<class T>
inline vec4<T> vec4<T>::operator-(const vec4<T> & minuend)
{
	return vec4(m_x - minuend.m_x, m_y - minuend.m_y, m_z - minuend.m_z, m_w - minuend.m_w);
}

template<class T>
inline T vec4<T>::dot(const vec4<T>& multiplicand)
{
	return m_x * multiplicand.m_x + m_y * multiplicand.m_y + m_z * multiplicand.m_z + m_w * multiplicand.m_w;
}

template<class T>
inline vec4<T> vec4<T>::cross(const vec4<T>& rhs)
{
	return vec4<T>(m_y * rhs.m_z - m_z * rhs.m_x, m_z * rhs.m_x - m_x * rhs.m_z, m_x * rhs.m_y - m_y * rhs.m_x, 0);
}
