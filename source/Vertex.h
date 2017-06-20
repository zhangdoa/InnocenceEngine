#pragma once
#include "stdafx.h"
#include "Vec2f.h"
#include "Vec3f.h"

class Vertex
{
public:
	Vertex();
	Vertex(Vec3f pos);
	Vertex(Vec3f pos, Vec2f texCoord);
	Vertex(Vec3f pos, Vec2f texCoord, Vec3f normal);
	~Vertex();

	static const int SIZE = 8;

	Vec3f* getPos();
	Vec2f* getTexCoord();
	Vec3f* getNormal();
	void setNormal(const Vec3f& normal);

private:
	Vec3f _pos;
	Vec2f _texCoord;
	Vec3f _normal;
};

