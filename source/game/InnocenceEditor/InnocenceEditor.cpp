#include "InnocenceEditor.h"

#include "../../engine/system/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

IGameInstance* g_pGameInstance;

namespace InnocenceEditorNS
{
	objectStatus m_objectStatus = objectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT InnocenceEditor::InnocenceEditor(void)
{
	g_pGameInstance = this;
}

INNO_SYSTEM_EXPORT bool InnocenceEditor::setup()
{
	InnocenceEditorNS::m_objectStatus = objectStatus::ALIVE;
}

INNO_SYSTEM_EXPORT bool InnocenceEditor::initialize()
{
}

INNO_SYSTEM_EXPORT bool InnocenceEditor::update()
{
}

INNO_SYSTEM_EXPORT bool InnocenceEditor::terminate()
{
	InnocenceEditorNS::m_objectStatus = objectStatus::SHUTDOWN;
}

INNO_SYSTEM_EXPORT objectStatus InnocenceEditor::getStatus()
{
	return InnocenceEditorNS::m_objectStatus;
}

INNO_SYSTEM_EXPORT std::string InnocenceEditor::getGameName()
{
	return std::string("InnocenceEditor");
}