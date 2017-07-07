#include "stdafx.h"
#include "IGameData.h"


IGameData::IGameData()
{
}


IGameData::~IGameData()
{
}

void IGameData::setInputManager(InputManager * inputManager)
{
	m_inputManager = inputManager;
}

InputManager * IGameData::getInputManager()
{
	return m_inputManager;
}
