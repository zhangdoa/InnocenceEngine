#include "Util.h"



Util::Util()
{
}


Util::~Util()
{
}

std::vector<int> Util::createIntBuffer(int size)
{
	return std::vector<int>(size);
}

std::vector<std::string> Util::removeEmptyStrings(const std::vector<std::string>& data)
{
	std::vector<std::string> res;
	for (int i = 0; i < data.size(); i++)
	{
		if (data[i] != "")
		{
			res.push_back(data[i]);
		}
			
	}
	return res;
}


std::vector<std::string> Util::split(const std::string& data, char marker)
{
	std::vector<std::string> elems;

	const char* cstr = data.c_str();
	unsigned int strLength = (unsigned int)data.length();
	unsigned int start = 0;
	unsigned int end = 0;

	while (end <= strLength)
	{
		while (end <= strLength)
		{
			if (cstr[end] == marker)
				break;
			end++;
		}

		elems.push_back(data.substr(start, end - start));
		start = end + 1;
		end = start;
	}

	return elems;
}
