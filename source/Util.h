#pragma once
#include "stdafx.h"
#include "Vertex.h"
#include "Mat4f.h"

class Util
{
public:
	Util();
	~Util();
	std::vector<int> createIntBuffer(int size);
	static std::vector<float> createFlippedBuffer(const Mat4f& value);
	std::vector<std::string> removeEmptyStrings(const std::vector<std::string>& data);
	static std::vector<std::string> split(const std::string& data, char marker);
};

