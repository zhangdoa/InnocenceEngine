#pragma once
#include "../../exports/HighLevelSystem_Export.h"
#include "../../common/InnoType.h"
#include "IGuiSystem.h"

class GLGuiSystem : public IGuiSystem
{
public:
	InnoHighLevelSystem_EXPORT bool setup() override;
	InnoHighLevelSystem_EXPORT bool initialize() override;
	InnoHighLevelSystem_EXPORT bool update() override;
	InnoHighLevelSystem_EXPORT bool terminate() override;

	InnoHighLevelSystem_EXPORT objectStatus getStatus() override;

private:
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
};

