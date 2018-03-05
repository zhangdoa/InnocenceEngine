#pragma once
#include "BaseManager.h"

#include "platform/InnoManagerHeader.h"

#ifdef INNO_PLATFORM_WIN32
#include "platform/InnoManagerHeaderWin32.h"
#endif

#include "interface/IGame.h"

class CoreManager : public BaseManager
{
public:
	CoreManager() {};
	~CoreManager() {};
	void setup() override;
	void initialize() override;
	void update() override;
	void shutdown() override;

private:
	std::vector<std::unique_ptr<IManager>> m_childEventManager;
};

