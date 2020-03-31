#include "innopropertyeditor.h"

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

InnoPropertyEditor::InnoPropertyEditor(QWidget *parent) : QWidget(parent)
{
}

void InnoPropertyEditor::initialize()
{
	m_transformComponentPropertyEditor = new TransformComponentPropertyEditor();
	m_transformComponentPropertyEditor->initialize();

    m_visibleComponentPropertyEditor = new VisibleComponentPropertyEditor();
    m_visibleComponentPropertyEditor->initialize();

	m_lightComponentPropertyEditor = new LightComponentPropertyEditor();
	m_lightComponentPropertyEditor->initialize();

	this->layout()->setAlignment(Qt::AlignTop);

	this->layout()->addWidget(m_transformComponentPropertyEditor);
    this->layout()->addWidget(m_visibleComponentPropertyEditor);
	this->layout()->addWidget(m_lightComponentPropertyEditor);
}

void InnoPropertyEditor::clear()
{
}

void InnoPropertyEditor::editComponent(int componentType, void *componentPtr)
{
	remove();

    if (componentType == 1)
    {
        m_transformComponentPropertyEditor->edit(componentPtr);
    }
    else if (componentType == 2)
    {
        m_visibleComponentPropertyEditor->edit(componentPtr);
    }
    else if (componentType == 3)
    {
        m_lightComponentPropertyEditor->edit(componentPtr);
    }
}

void InnoPropertyEditor::remove()
{
	m_transformComponentPropertyEditor->remove();
    m_visibleComponentPropertyEditor->remove();
	m_lightComponentPropertyEditor->remove();
}
