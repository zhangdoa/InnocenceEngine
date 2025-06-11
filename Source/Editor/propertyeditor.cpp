#include "propertyeditor.h"

#include "../Engine/Engine.h"

using namespace Inno;


PropertyEditor::PropertyEditor(QWidget* parent) : QWidget(parent)
{
}

void PropertyEditor::initialize()
{
    m_modelComponentPropertyEditor = new ModelComponentPropertyEditor();
    m_modelComponentPropertyEditor->initialize();

    m_lightComponentPropertyEditor = new LightComponentPropertyEditor();
    m_lightComponentPropertyEditor->initialize();

    m_cameraComponentPropertyEditor = new CameraComponentPropertyEditor();
    m_cameraComponentPropertyEditor->initialize();

    this->layout()->setAlignment(Qt::AlignTop);

    this->layout()->addWidget(m_modelComponentPropertyEditor);
    this->layout()->addWidget(m_lightComponentPropertyEditor);
    this->layout()->addWidget(m_cameraComponentPropertyEditor);
}

void PropertyEditor::clear()
{
}

void PropertyEditor::editComponent(int componentType, void* componentPtr)
{
    remove();

    if (componentType == ModelComponent::GetTypeID())
    {
        m_modelComponentPropertyEditor->edit(componentPtr);
    }
    else if (componentType == LightComponent::GetTypeID())
    {
        m_lightComponentPropertyEditor->edit(componentPtr);
    }
    else if (componentType == CameraComponent::GetTypeID())
    {
        m_cameraComponentPropertyEditor->edit(componentPtr);
    }
}

void PropertyEditor::remove()
{
    m_modelComponentPropertyEditor->remove();
    m_lightComponentPropertyEditor->remove();
    m_cameraComponentPropertyEditor->remove();
}
