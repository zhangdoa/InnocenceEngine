#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoConcurrency.h"
#include "../../exports/LowLevelSystem_Export.h"

namespace InnoTaskSystem
{
	InnoLowLevelSystem_EXPORT void setup();
	InnoLowLevelSystem_EXPORT void initialize();
	InnoLowLevelSystem_EXPORT void update();
	InnoLowLevelSystem_EXPORT void shutdown();

    static InnoThreadPool m_threadPool;
    
	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		return m_threadPool.submit(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};

