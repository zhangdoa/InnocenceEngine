#pragma once

#include "InnoSystemHeader.h"
#include "System/MemorySystem.h"
#include "System/LogSystem.h"
#include "System/TaskSystem.h"
#include "System/TimeSystem.h"
#include "System/RenderingSystem.h"
#include "System/AssetSystem.h"
#include "System/GameSystem.h"

#ifdef INNO_PLATFORM_WIN32
#define INNO_MEMORY_SYSTEM MemorySystem
#define INNO_LOG_SYSTEM LogSystem
#define INNO_TASK_SYSTEM TaskSystem
#define INNO_TIME_SYSTEM TimeSystem
#define INNO_RENDERING_SYSTEM RenderingSystem
#define INNO_ASSET_SYSTEM AssetSystem
#define INNO_GAME_SYSTEM GameSystem
#endif


