#pragma once

#include "interface/IMemorySystem.h"
#include "interface/ILogSystem.h"
#include "interface/ITaskSystem.h"
#include "interface/ITimeSystem.h"
#include "interface/IGameSystem.h"
#include "interface/IAssetSystem.h"
#include "interface/IPhysicsSystem.h"
#include "interface/IVisionSystem.h"

#if defined(INNO_PLATFORM_WIN32)
#include "platform/InnoSystemHeaderWin32.h"
#elif defined(INNO_PLATFORM_WIN64)
#include "platform/InnoSystemHeaderWin64.h"
#elif defined(INNO_PLATFORM_LINUX64)
#include "platform/InnoSystemHeaderLinux64.h"
#elif defined(INNO_PLATFORM_MACOS)
#include "platform/InnoSystemHeaderMacOS.h"
#else
#endif
#if defined(BUILD_EDITOR)
#define INNO_GAME_SYSTEM InnocenceEditor
#include "../game/InnocenceEditor/InnocenceEditor.h"
#elif defined(BUILD_GAME)
#define INNO_GAME_SYSTEM InnocenceGarden
#include "../game/InnocenceGarden/InnocenceGarden.h"
#elif defined(BUILD_TEST)
#define INNO_GAME_SYSTEM InnocenceTest
#include "../game/InnocenceTest/InnocenceTest.h"
#else
#endif