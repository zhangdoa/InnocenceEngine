#pragma once

#include "IObject.hpp"

#ifndef _I_GAME_DATA_H_
#define _I_GAME_DATA_H_

class IGameData : public IObject
{
public:
	virtual ~IGameData() {};
	void getGameName(std::string& gameName) const;
	bool needRender = true;
};

#endif // !_I_GAME_DATA_H_
