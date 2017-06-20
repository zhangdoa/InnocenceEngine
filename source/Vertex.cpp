#include "Vertex.h"



Vertex::Vertex()
{
	_pos = Vec3f(0.0f, 0.0f, 0.0f);
	_texCoord = Vec2f(0.0f, 0.0f);
	_normal = Vec3f(0.0f, 0.0f, 0.0f);
}

Vertex::Vertex(Vec3f pos)
{
	_pos = pos;
	_texCoord = Vec2f(0.0f, 0.0f);
	_normal = Vec3f(0.0f, 0.0f, 0.0f);
}

Vertex::Vertex(Vec3f pos, Vec2f texCoord)
{
	_pos = pos;
	_texCoord = texCoord;
	_normal = Vec3f(0.0f, 0.0f, 0.0f);
}

Vertex::Vertex(Vec3f pos, Vec2f texCoord, Vec3f normal)
{
	_pos = pos;
	_texCoord = texCoord;
	_normal = normal;
}


Vertex::~Vertex()
{
}

Vec3f* Vertex::getPos()
{
	return &_pos;
}

Vec2f* Vertex::getTexCoord()
{
	return &_texCoord;
}

Vec3f* Vertex::getNormal()
{
	return &_normal;
}

void Vertex::setNormal(const Vec3f& normal)
{
	this->_normal = normal;
}
