#pragma once
#include "../../Engine/Core/ILogicClient.h"

class DefaultLogicClient : public ILogicClient
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(DefaultLogicClient);

	bool setup() override;
	bool initialize() override;
	bool update() override;
	bool terminate() override;

	ObjectStatus getStatus() override;

	std::string getApplicationName() override;
};
