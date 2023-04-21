#include "propertyeditor.h"

#include "../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

PropertyEditor::PropertyEditor(QWidget *parent) : QWidget(parent)
{
}

void PropertyEditor::initialize()
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

void PropertyEditor::clear()
{
}

void PropertyEditor::editComponent(int componentType, void *componentPtr)
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

void PropertyEditor::remove()
{
    m_transformComponentPropertyEditor->remove();
    m_visibleComponentPropertyEditor->remove();
    m_lightComponentPropertyEditor->remove();
    m_cameraComponentPropertyEditor->remove();
}
