#include "stdafx.h"
#include "IGameData.h"


IGameData::IGameData()
{
}


IGameData::~IGameData()
{
}

void IGameData::getGameName(std::string & gameName) const
{
	gameName = typeid(*this).name();
}