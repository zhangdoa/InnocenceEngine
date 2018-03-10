#pragma once

#include "InnoManagerHeader.h"
#include "manager/MemoryManager.h"
#include "manager/LogManager.h"
#include "manager/TaskManager.h"
#include "manager/TimeManager.h"
#include "manager/graphic/RenderingManager.h"
#include "manager/AssetManager.h"
#include "manager/GameManager.h"

#ifdef INNO_PLATFORM_WIN32
#define INNO_MEMORY_MANAGER MemoryManager
#define INNO_LOG_MANAGER LogManager
#define INNO_TASK_MANAGER TaskManager
#define INNO_TIME_MANAGER TimeManager
#define INNO_RENDERING_MANAGER RenderingManager
#define INNO_ASSET_MANAGER AssetManager
#define INNO_GAME_MANAGER GameManager
#endif


