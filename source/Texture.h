#pragma once
#include "stdafx.h"
class Texture
{
public:
	Texture();
	Texture(std::string fileName);
	~Texture();

	int getId();

	void bind();
private:
	int id;
	static int loadTexture(std::string fileName);

};

