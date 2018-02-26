#pragma once
#include "interface/IGame.h"

class BaseGame : public IGame
{
public:
	BaseGame() {};
	~BaseGame() {};

	const objectStatus& getStatus() const;

	void BaseGame::getGameName(std::string & gameName) const override
	{
		gameName = std::string{ typeid(*this).name() }.substr(std::string{ typeid(*this).name() }.find("class"), std::string::npos);
	}

protected:
	void setStatus(objectStatus objectStatus);

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

