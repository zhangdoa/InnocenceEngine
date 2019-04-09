#include "innopropertyeditor.h"

#include "transformcomponentpropertyeditor.h"

#include "../../engine/system/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;


InnoPropertyEditor::InnoPropertyEditor(QWidget *parent) : QWidget(parent)
{

}

void InnoPropertyEditor::initialize()
{
    m_transformComponentPropertyEditor = new TransformComponentPropertyEditor();
    m_transformComponentPropertyEditor->initialize();

    this->layout()->setAlignment(Qt::AlignTop);

    this->layout()->addWidget(m_transformComponentPropertyEditor);
}

void InnoPropertyEditor::clear()
{

}

void InnoPropertyEditor::editComponent(int componentType, void *componentPtr)
{
    //g_pCoreSystem->getLogSystem()->printLog(LogType::INNO_ERROR, std::to_string(componentType));
}
