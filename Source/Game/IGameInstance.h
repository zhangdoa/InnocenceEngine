#pragma once
#include "../Engine/Common/InnoType.h"
#include "../Engine/Common/InnoClassTemplate.h"
#include "../Engine/Common/ComponentHeaders.h"

INNO_INTERFACE IGameInstance
{
public:
	INNO_CLASS_INTERFACE_NON_COPYABLE(IGameInstance);

	virtual bool setup() = 0;
	virtual bool initialize() = 0;
	virtual bool update() = 0;
	virtual bool terminate() = 0;

	virtual ObjectStatus getStatus() = 0;

	virtual std::string getGameName() = 0;
};
