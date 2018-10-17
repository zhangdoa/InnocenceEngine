#pragma once
#include "../../common/InnoType.h"
#include "../../common/InnoConcurrency.h"

namespace InnoTaskSystem
{
	__declspec(dllexport) void setup();
	__declspec(dllexport) void initialize();
	__declspec(dllexport) void update();
	__declspec(dllexport) void shutdown();

    static InnoThreadPool m_threadPool;
    
	template <typename Func, typename... Args>
	__declspec(dllexport) auto submit(Func&& func, Args&&... args)
	{
		return m_threadPool.submit(std::forward<Func>(func), std::forward<Args>(args)...);
	}

	__declspec(dllexport) objectStatus getStatus();
};

