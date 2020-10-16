#pragma once
#include "../../Engine/Interface/ILogicClient.h"

class DefaultLogicClient : public ILogicClient
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(DefaultLogicClient);

	bool Setup(ISystemConfig* systemConfig) override;
	bool Initialize() override;
	bool Update() override;
	bool Terminate() override;

	ObjectStatus GetStatus() override;

	std::string getApplicationName() override;
};
