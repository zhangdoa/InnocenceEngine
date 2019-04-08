#include "innopropertyeditor.h"

#include "../../engine/system/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;


InnoPropertyEditor::InnoPropertyEditor(QWidget *parent) : QWidget(parent)
{

}

void InnoPropertyEditor::initialize()
{

}

void InnoPropertyEditor::clear()
{

}

void InnoPropertyEditor::editComponent(int componentType, void *componentPtr)
{
    //g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, std::to_string(componentType));
}
