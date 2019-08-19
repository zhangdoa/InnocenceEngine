#include "materialdatacomponentpropertyeditor.h"

#include "innopropertyeditor.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

MaterialDataComponentPropertyEditor::MaterialDataComponentPropertyEditor()
{
}

void MaterialDataComponentPropertyEditor::initialize()
{
    m_gridLayout = new QGridLayout();
    m_gridLayout->setMargin(4);

    m_validator = new QDoubleValidator(-2147483647, 2147483647, 4);
    m_validator->setProperty("notation", QDoubleValidator::StandardNotation);

    m_title = new QLabel("MaterialDataComponent");
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

    m_gridLayout->addWidget(m_line, row, 0, 1, 7);

    connect(m_albedoR, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));
    connect(m_albedoG, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));
    connect(m_albedoB, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));
    connect(m_alpha, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));
    connect(m_metallic, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));
    connect(m_roughness, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));
    connect(m_AO, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));
    connect(m_thickness, SIGNAL(ValueChanged()), this, SLOT(SetCustomMaterial()));

    m_gridLayout->setHorizontalSpacing(m_horizontalSpacing);
    m_gridLayout->setVerticalSpacing(m_verticalSpacing);

    this->setLayout(m_gridLayout);
    this->hide();
}

void MaterialDataComponentPropertyEditor::edit(void *component)
{
    m_component = reinterpret_cast<MaterialDataComponent*>(component);

    GetCustomMaterial();
    this->show();
}

void MaterialDataComponentPropertyEditor::GetCustomMaterial()
{
    if (!m_component)
        return;

    m_albedoR->SetFromFloat(m_component->m_meshCustomMaterial.AlbedoR);
    m_albedoG->SetFromFloat(m_component->m_meshCustomMaterial.AlbedoG);
    m_albedoB->SetFromFloat(m_component->m_meshCustomMaterial.AlbedoB);
    m_alpha->SetFromFloat(m_component->m_meshCustomMaterial.Alpha);
    m_metallic->SetFromFloat(m_component->m_meshCustomMaterial.Metallic);
    m_roughness->SetFromFloat(m_component->m_meshCustomMaterial.Roughness);
    m_AO->SetFromFloat(m_component->m_meshCustomMaterial.AO);
    m_thickness->SetFromFloat(m_component->m_meshCustomMaterial.Thickness);
}

void MaterialDataComponentPropertyEditor::SetCustomMaterial()
{
    if (!m_component)
        return;

    m_component->m_meshCustomMaterial.AlbedoR = m_albedoR->GetAsFloat();
    m_component->m_meshCustomMaterial.AlbedoG = m_albedoG->GetAsFloat();
    m_component->m_meshCustomMaterial.AlbedoB = m_albedoB->GetAsFloat();
    m_component->m_meshCustomMaterial.Alpha = m_alpha->GetAsFloat();
    m_component->m_meshCustomMaterial.Metallic = m_metallic->GetAsFloat();
    m_component->m_meshCustomMaterial.Roughness = m_roughness->GetAsFloat();
    m_component->m_meshCustomMaterial.AO = m_AO->GetAsFloat();
    m_component->m_meshCustomMaterial.Thickness = m_thickness->GetAsFloat();
}

void MaterialDataComponentPropertyEditor::remove()
{
    m_component = nullptr;
    this->hide();
}
