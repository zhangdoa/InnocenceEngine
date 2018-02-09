#include "../../main/stdafx.h"
#include "IGameData.h"

void IGameData::getGameName(std::string & gameName) const
{
	gameName = std::string{ typeid(*this).name() }.substr(std::string{ typeid(*this).name() }.find("class"), std::string::npos);
}