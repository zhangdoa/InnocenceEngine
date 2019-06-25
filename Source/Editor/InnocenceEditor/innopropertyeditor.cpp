#include "innopropertyeditor.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

InnoPropertyEditor::InnoPropertyEditor(QWidget *parent) : QWidget(parent)
{
}

void InnoPropertyEditor::initialize()
{
    m_transformComponentPropertyEditor = new TransformComponentPropertyEditor();
    m_transformComponentPropertyEditor->initialize();

    m_directionalLightComponentPropertyEditor = new DirectionalLightComponentPropertyEditor();
    m_directionalLightComponentPropertyEditor->initialize();

    m_pointLightComponentPropertyEditor = new PointLightComponentPropertyEditor();
    m_pointLightComponentPropertyEditor->initialize();

    m_sphereLightComponentPropertyEditor = new SphereLightComponentPropertyEditor();
    m_sphereLightComponentPropertyEditor->initialize();

    this->layout()->setAlignment(Qt::AlignTop);

    this->layout()->addWidget(m_transformComponentPropertyEditor);
    this->layout()->addWidget(m_directionalLightComponentPropertyEditor);
    this->layout()->addWidget(m_pointLightComponentPropertyEditor);
    this->layout()->addWidget(m_sphereLightComponentPropertyEditor);
}

void InnoPropertyEditor::clear()
{
}

void InnoPropertyEditor::editComponent(int componentType, void *componentPtr)
{
    remove();

    enum ComponentType l_type;
    l_type = ComponentType(componentType);
    switch (l_type)
    {
    case ComponentType::TransformComponent:
        m_transformComponentPropertyEditor->edit(componentPtr);
        break;
    case ComponentType::DirectionalLightComponent:
        m_directionalLightComponentPropertyEditor->edit(componentPtr);
        break;
    case ComponentType::PointLightComponent:
        m_pointLightComponentPropertyEditor->edit(componentPtr);
        break;
    case ComponentType::SphereLightComponent:
        m_sphereLightComponentPropertyEditor->edit(componentPtr);
        break;
    }
}

void InnoPropertyEditor::remove()
{
    m_transformComponentPropertyEditor->remove();
    m_directionalLightComponentPropertyEditor->remove();
    m_pointLightComponentPropertyEditor->remove();
    m_sphereLightComponentPropertyEditor->remove();
}
