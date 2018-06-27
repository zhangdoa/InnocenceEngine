#pragma once

#include "InnoSystemHeader.h"
#include "system/MemorySystem.h"
#include "system/LogSystem.h"
#include "system/TaskSystem.h"
#include "system/TimeSystem.h"
#include "system/GameSystem.h"
#include "system/AssetSystem.h"
#include "system/PhysicsSystem.h"
#include "system/VisionSystem.h"

#ifdef INNO_PLATFORM_WIN64
#define INNO_MEMORY_SYSTEM MemorySystem
#define INNO_LOG_SYSTEM LogSystem
#define INNO_TASK_SYSTEM TaskSystem
#define INNO_TIME_SYSTEM TimeSystem
#define INNO_GAME_SYSTEM GameSystem
#define INNO_ASSET_SYSTEM AssetSystem
#define INNO_VISION_SYSTEM VisionSystem
#define INNO_PHYSICS_SYSTEM PhysicsSystem
#endif
