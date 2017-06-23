#pragma once
#include "stdafx.h"
#include "Math.h"

class Util
{
public:
	Util();
	~Util();
	std::vector<int> createIntBuffer(int size);
	std::vector<std::string> removeEmptyStrings(const std::vector<std::string>& data);
	static std::vector<std::string> split(const std::string& data, char marker);
};

