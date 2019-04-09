#include "innopropertyeditor.h"

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
    enum ComponentType l_type;
    l_type = ComponentType(componentType);
    switch (l_type)
    {
        case ComponentType::TransformComponent:
        m_transformComponentPropertyEditor->edit(componentPtr);
        break;
    }
}

void InnoPropertyEditor::remove()
{
    m_transformComponentPropertyEditor->remove();
}
