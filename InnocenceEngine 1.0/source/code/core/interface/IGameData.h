#pragma once

#include "IObject.h"

#ifndef _I_GAME_DATA_H_
#define _I_GAME_DATA_H_

class IGameData : public IObject
{
public:
	IGameData();
	virtual ~IGameData();
	void getGameName(std::string& gameName) const;
};

#endif // !_I_GAME_DATA_H_
