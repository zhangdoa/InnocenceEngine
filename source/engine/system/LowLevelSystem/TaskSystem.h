#pragma once
#include "../../exports/LowLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "../../common/InnoConcurrency.h"

#include "../../component/TaskSystemSingletonComponent.h"

namespace InnoTaskSystem
{
	InnoLowLevelSystem_EXPORT bool setup();
	InnoLowLevelSystem_EXPORT bool initialize();
	InnoLowLevelSystem_EXPORT bool update();
	InnoLowLevelSystem_EXPORT bool terminate();
    
	template <typename Func, typename... Args>
	auto submit(Func&& func, Args&&... args)
	{
		return TaskSystemSingletonComponent::getInstance().m_threadPool.submit(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	InnoLowLevelSystem_EXPORT objectStatus getStatus();
};

