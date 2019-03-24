#include "CoreSystem.h"
#include "TimeSystem.h"
#include "LogSystem.h"
#include "MemorySystem.h"
#include "TaskSystem.h"
#include "FileSystem.h"
#include "GameSystem.h"
#include "AssetSystem.h"
#include "PhysicsSystem.h"
#include "InputSystem.h"
#include "VisionSystem.h"

ICoreSystem* g_pCoreSystem;

INNO_PRIVATE_SCOPE InnoCoreSystemNS
{
	bool createSubSystemInstance();
	bool setup(void* hInstance, void* hwnd, char* pScmdline);
	bool initialize();
	bool update();
	bool terminate();

	std::unique_ptr<ITimeSystem> m_TimeSystem;
	std::unique_ptr<ILogSystem> m_LogSystem;
	std::unique_ptr<IMemorySystem> m_MemorySystem;
	std::unique_ptr<ITaskSystem> m_TaskSystem;
	std::unique_ptr<IFileSystem> m_FileSystem;
	std::unique_ptr<IGameSystem> m_GameSystem;
	std::unique_ptr<IAssetSystem> m_AssetSystem;
	std::unique_ptr<IPhysicsSystem> m_PhysicsSystem;
	std::unique_ptr<IInputSystem> m_InputSystem;
	std::unique_ptr<IVisionSystem> m_VisionSystem;
}

bool InnoCoreSystemNS::createSubSystemInstance()
{
	m_TimeSystem = std::make_unique<InnoTimeSystem>();
	if (!m_TimeSystem.get())
	{
		return false;
	}
	m_LogSystem = std::make_unique<InnoLogSystem>();
	if (!m_LogSystem.get())
	{
		return false;
	}
	m_MemorySystem = std::make_unique<InnoMemorySystem>();
	if (!m_MemorySystem.get())
	{
		return false;
	}
	m_TaskSystem = std::make_unique<InnoTaskSystem>();
	if (!m_TaskSystem.get())
	{
		return false;
	}
	m_FileSystem = std::make_unique<InnoFileSystem>();
	if (!m_FileSystem.get())
	{
		return false;
	}
	m_GameSystem = std::make_unique<InnoGameSystem>();
	if (!m_GameSystem.get())
	{
		return false;
	}
	m_AssetSystem = std::make_unique<InnoAssetSystem>();
	if (!m_AssetSystem.get())
	{
		return false;
	}
	m_PhysicsSystem = std::make_unique<InnoPhysicsSystem>();
	if (!m_PhysicsSystem.get())
	{
		return false;
	}
	m_InputSystem = std::make_unique<InnoInputSystem>();
	if (!m_InputSystem.get())
	{
		return false;
	}
	m_VisionSystem = std::make_unique<InnoVisionSystem>();
	if (!m_VisionSystem.get())
	{
		return false;
	}
	return true;
}

bool InnoCoreSystemNS::setup(void* hInstance, void* hwnd, char* pScmdline)
{
	if (!InnoCoreSystemNS::createSubSystemInstance())
	{
		return false;
	}

	if (!g_pCoreSystem->getTimeSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TimeSystem setup finished.");

	if (!g_pCoreSystem->getLogSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "LogSystem setup finished.");

	if (!g_pCoreSystem->getMemorySystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "MemorySystem setup finished.");

	if (!g_pCoreSystem->getTaskSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "TaskSystem setup finished.");

	if (!g_pCoreSystem->getVisionSystem()->setup(hInstance, hwnd, pScmdline))
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "VisionSystem setup finished.");

	if (!g_pCoreSystem->getFileSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "FileSystem setup finished.");

	if (!g_pCoreSystem->getGameSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "GameSystem setup finished.");

	if (!g_pCoreSystem->getAssetSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "AssetSystem setup finished.");

	if (!g_pCoreSystem->getPhysicsSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "PhysicsSystem setup finished.");

	if (!g_pCoreSystem->getInputSystem()->setup())
	{
		return false;
	}
	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "InputSystem setup finished.");

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine setup finished.");
	return true;
}

bool InnoCoreSystemNS::initialize()
{
	if (!g_pCoreSystem->getTimeSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getLogSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getMemorySystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getTaskSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getFileSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getAssetSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getPhysicsSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getInputSystem()->initialize())
	{
		return false;
	}

	if (!g_pCoreSystem->getVisionSystem()->initialize())
	{
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been initialized.");
	return true;
}

bool InnoCoreSystemNS::update()
{
	if (!g_pCoreSystem->getTimeSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getLogSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getMemorySystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getTaskSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getFileSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getAssetSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getPhysicsSystem()->update())
	{
		return false;
	}

	if (!g_pCoreSystem->getInputSystem()->update())
	{
		return false;
	}

	if (g_pCoreSystem->getVisionSystem()->getStatus() == ObjectStatus::ALIVE)
	{
		if (!g_pCoreSystem->getVisionSystem()->update())
		{
			return false;
		}
		g_pCoreSystem->getGameSystem()->saveComponentsCapture();
	}
	else
	{
		g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_WARNING, "Engine is stand-by.");
		return false;
	}

	return true;
}

bool InnoCoreSystemNS::terminate()
{
	if (!g_pCoreSystem->getVisionSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getInputSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getPhysicsSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getAssetSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getGameSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getFileSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getTaskSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getMemorySystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getLogSystem()->terminate())
	{
		return false;
	}

	if (!g_pCoreSystem->getTimeSystem()->terminate())
	{
		return false;
	}

	g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_DEV_SUCCESS, "Engine has been terminated.");

	return true;
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::setup(void* hInstance, void* hwnd, char* pScmdline)
{
	g_pCoreSystem = this;

	return InnoCoreSystemNS::setup(hInstance, hwnd, pScmdline);
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::initialize()
{
	return InnoCoreSystemNS::initialize();
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::update()
{
	return InnoCoreSystemNS::update();
}

INNO_SYSTEM_EXPORT bool InnoCoreSystem::terminate()
{
	return InnoCoreSystemNS::terminate();
}

INNO_SYSTEM_EXPORT ITimeSystem * InnoCoreSystem::getTimeSystem()
{
	return InnoCoreSystemNS::m_TimeSystem.get();
}

INNO_SYSTEM_EXPORT ILogSystem * InnoCoreSystem::getLogSystem()
{
	return InnoCoreSystemNS::m_LogSystem.get();
}

INNO_SYSTEM_EXPORT IMemorySystem * InnoCoreSystem::getMemorySystem()
{
	return InnoCoreSystemNS::m_MemorySystem.get();
}

INNO_SYSTEM_EXPORT ITaskSystem * InnoCoreSystem::getTaskSystem()
{
	return InnoCoreSystemNS::m_TaskSystem.get();
}

INNO_SYSTEM_EXPORT IFileSystem * InnoCoreSystem::getFileSystem()
{
	return 	InnoCoreSystemNS::m_FileSystem.get();
}

INNO_SYSTEM_EXPORT IGameSystem * InnoCoreSystem::getGameSystem()
{
	return 	InnoCoreSystemNS::m_GameSystem.get();
}

INNO_SYSTEM_EXPORT IAssetSystem * InnoCoreSystem::getAssetSystem()
{
	return InnoCoreSystemNS::m_AssetSystem.get();
}

INNO_SYSTEM_EXPORT IPhysicsSystem * InnoCoreSystem::getPhysicsSystem()
{
	return InnoCoreSystemNS::m_PhysicsSystem.get();
}

INNO_SYSTEM_EXPORT IInputSystem * InnoCoreSystem::getInputSystem()
{
	return InnoCoreSystemNS::m_InputSystem.get();
}

INNO_SYSTEM_EXPORT IVisionSystem * InnoCoreSystem::getVisionSystem()
{
	return InnoCoreSystemNS::m_VisionSystem.get();
}