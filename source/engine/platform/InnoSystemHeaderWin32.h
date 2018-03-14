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
#define INNO_MEMORY_System MemorySystem
#define INNO_LOG_System LogSystem
#define INNO_TASK_System TaskSystem
#define INNO_TIME_System TimeSystem
#define INNO_RENDERING_System RenderingSystem
#define INNO_ASSET_System AssetSystem
#define INNO_GAME_System GameSystem
#endif


