#include "innopropertyeditor.h"

#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

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

    m_cameraComponentPropertyEditor = new CameraComponentPropertyEditor();
    m_cameraComponentPropertyEditor->initialize();

    this->layout()->setAlignment(Qt::AlignTop);

    this->layout()->addWidget(m_transformComponentPropertyEditor);
    this->layout()->addWidget(m_visibleComponentPropertyEditor);
    this->layout()->addWidget(m_lightComponentPropertyEditor);
    this->layout()->addWidget(m_cameraComponentPropertyEditor);

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
    else if (componentType == 4)
    {
        m_cameraComponentPropertyEditor->edit(componentPtr);
    }
}

void InnoPropertyEditor::remove()
{
    m_transformComponentPropertyEditor->remove();
    m_visibleComponentPropertyEditor->remove();
    m_lightComponentPropertyEditor->remove();
    m_cameraComponentPropertyEditor->remove();
}
