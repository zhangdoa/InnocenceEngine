#pragma once
#include "ITaskSystem.h"

class InnoTaskSystem : INNO_IMPLEMENT ITaskSystem
{
public:
	INNO_CLASS_CONCRETE_NON_COPYABLE(InnoTaskSystem);

	INNO_SYSTEM_EXPORT bool setup() override;
	INNO_SYSTEM_EXPORT bool initialize() override;
	INNO_SYSTEM_EXPORT bool update() override;
	INNO_SYSTEM_EXPORT bool terminate() override;
    
	INNO_SYSTEM_EXPORT ObjectStatus getStatus()  override;

	INNO_SYSTEM_EXPORT void addTask(std::unique_ptr<IThreadTask>&& task) override;
};

