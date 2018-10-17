#pragma once
#include "../../exports/LowLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "../../common/InnoConcurrency.h"

#include "../../component/TaskSystemSingletonComponent.h"

namespace InnoTaskSystem
{
	InnoLowLevelSystem_EXPORT void setup();
	InnoLowLevelSystem_EXPORT void initialize();
	InnoLowLevelSystem_EXPORT void update();
	InnoLowLevelSystem_EXPORT void shutdown();
    
	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		return TaskSystemSingletonComponent::getInstance().m_threadPool.submit(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};

