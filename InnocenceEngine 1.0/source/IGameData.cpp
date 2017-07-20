#include "stdafx.h"
#include "IGameData.h"


IGameData::IGameData()
{
}


IGameData::~IGameData()
{
}

const std::string IGameData::getGameName()
{
	std::string l_gameName = typeid(*this).name();
	return l_gameName;
}
