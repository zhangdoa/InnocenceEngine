#include "materialComponentpropertyeditor.h"

#include "propertyeditor.h"

#include "../Engine/Engine.h"

using namespace Inno;


MaterialComponentPropertyEditor::MaterialComponentPropertyEditor()
{
}

void MaterialComponentPropertyEditor::initialize()
{
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(4, 4, 4, 4);

    m_title = new QLabel("MaterialComponent");
    m_title->setStyleSheet(
                "background-repeat: no-repeat;"
                "background-position: left;"
                );

    m_albedoLabel = new QLabel("Albedo");

    m_albedoR = new ComboLabelText();
    m_albedoR->Initialize("R");

    m_albedoG = new ComboLabelText();
    m_albedoG->Initialize("G");

    m_albedoB = new ComboLabelText();
    m_albedoB->Initialize("B");

    m_alpha = new ComboLabelText();
    m_alpha->Initialize("Alpha");

    m_MRATLabel = new QLabel("MRAT");

    m_metallic = new ComboLabelText();
    m_metallic->Initialize("Metallic");

    m_roughness = new ComboLabelText();
    m_roughness->Initialize("Roughness");

    m_AO = new ComboLabelText();
    m_AO->Initialize("Ambient Occlusion");

    m_thickness = new ComboLabelText();
    m_thickness->Initialize("Thickness");


    m_shaderModel = new ComboLabelText();
    m_shaderModel->Initialize("ShaderModel");

    m_line = new QWidget();
    m_line->setFixedHeight(1);
    m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_line->setStyleSheet(QString("background-color: #585858;"));

    int row = 0;
    m_gridLayout->addWidget(m_title, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_albedoLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_albedoR->GetLabelWidget(), row, 1, 1, 1);
    m_gridLayout->addWidget(m_albedoR->GetTextWidget(), row, 2, 1, 1);
    m_gridLayout->addWidget(m_albedoG->GetLabelWidget(), row, 3, 1, 1);
    m_gridLayout->addWidget(m_albedoG->GetTextWidget(), row, 4, 1, 1);
    m_gridLayout->addWidget(m_albedoB->GetLabelWidget(), row, 5, 1, 1);
    m_gridLayout->addWidget(m_albedoB->GetTextWidget(), row, 6, 1, 1);
    m_gridLayout->addWidget(m_alpha->GetLabelWidget(), row, 7, 1, 1);
    m_gridLayout->addWidget(m_alpha->GetTextWidget(), row, 8, 1, 1);
    row++;

    m_gridLayout->addWidget(m_MRATLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_metallic->GetLabelWidget(), row, 1, 1, 1);
    m_gridLayout->addWidget(m_metallic->GetTextWidget(), row, 2, 1, 1);
    m_gridLayout->addWidget(m_roughness->GetLabelWidget(), row, 3, 1, 1);
    m_gridLayout->addWidget(m_roughness->GetTextWidget(), row, 4, 1, 1);
    m_gridLayout->addWidget(m_AO->GetLabelWidget(), row, 5, 1, 1);
    m_gridLayout->addWidget(m_AO->GetTextWidget(), row, 6, 1, 1);
    m_gridLayout->addWidget(m_thickness->GetLabelWidget(), row, 7, 1, 1);
    m_gridLayout->addWidget(m_thickness->GetTextWidget(), row, 8, 1, 1);

    row++;

    m_gridLayout->addWidget(m_shaderModel->GetLabelWidget(), row, 1, 1, 1);
    m_gridLayout->addWidget(m_shaderModel->GetTextWidget(), row, 2, 1, 1);
    row++;


    m_gridLayout->addWidget(m_line, row, 0, 1, 7);

    connect(m_albedoR, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_albedoG, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_albedoB, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_alpha, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_metallic, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_roughness, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_AO, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_thickness, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));
    connect(m_shaderModel, SIGNAL(ValueChanged()), this, SLOT(SetMaterialAttributes()));

    m_gridLayout->setHorizontalSpacing(m_horizontalSpacing);
    m_gridLayout->setVerticalSpacing(m_verticalSpacing);

    this->setLayout(m_gridLayout);
    this->hide();
}

void MaterialComponentPropertyEditor::edit(void *component)
{
    m_component = reinterpret_cast<MaterialComponent*>(component);

    GetMaterialAttributes();
    this->show();
}

void MaterialComponentPropertyEditor::GetMaterialAttributes()
{
    if (!m_component)
        return;

    m_albedoR->SetFromFloat(m_component->m_materialAttributes.AlbedoR);
    m_albedoG->SetFromFloat(m_component->m_materialAttributes.AlbedoG);
    m_albedoB->SetFromFloat(m_component->m_materialAttributes.AlbedoB);
    m_alpha->SetFromFloat(m_component->m_materialAttributes.Alpha);
    m_metallic->SetFromFloat(m_component->m_materialAttributes.Metallic);
    m_roughness->SetFromFloat(m_component->m_materialAttributes.Roughness);
    m_AO->SetFromFloat(m_component->m_materialAttributes.AO);
    m_thickness->SetFromFloat(m_component->m_materialAttributes.Thickness);
    m_shaderModel->SetFromInt((int)m_component->m_ShaderModel);
}

void MaterialComponentPropertyEditor::SetMaterialAttributes()
{
    if (!m_component)
        return;

    m_component->m_materialAttributes.AlbedoR = m_albedoR->GetAsFloat();
    m_component->m_materialAttributes.AlbedoG = m_albedoG->GetAsFloat();
    m_component->m_materialAttributes.AlbedoB = m_albedoB->GetAsFloat();
    m_component->m_materialAttributes.Alpha = m_alpha->GetAsFloat();
    m_component->m_materialAttributes.Metallic = m_metallic->GetAsFloat();
    m_component->m_materialAttributes.Roughness = m_roughness->GetAsFloat();
    m_component->m_materialAttributes.AO = m_AO->GetAsFloat();
    m_component->m_materialAttributes.Thickness = m_thickness->GetAsFloat();
    m_component->m_ShaderModel = ShaderModel(m_shaderModel->GetAsInt());
}

void MaterialComponentPropertyEditor::remove()
{
    m_component = nullptr;
    this->hide();
}
