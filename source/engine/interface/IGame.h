#pragma once
#include "IObject.hpp"

class IGame : public IObject
{
public:
	virtual ~IGame() {};
	virtual void getGameName(std::string& gameName) const = 0;
	bool needRender = true;
};