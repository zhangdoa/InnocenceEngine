#include "sphereLightcomponentpropertyeditor.h"

#include "../../Engine/System/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

SphereLightComponentPropertyEditor::SphereLightComponentPropertyEditor()
{
}

void SphereLightComponentPropertyEditor::initialize()
{
    m_gridLayout = new QGridLayout();
    m_gridLayout->setMargin(4);

    m_validator = new QDoubleValidator(-2147483647, 2147483647, 4);
    m_validator->setProperty("notation", QDoubleValidator::StandardNotation);

    m_title = new QLabel("SphereLightComponent");
    m_title->setStyleSheet(
                "background-repeat: no-repeat;"
                "background-position: left;"
                "padding-left: 20px;"
                );

    m_sphereRadiusLabel = new QLabel("Sphere Radius");

    m_sphereRadius = new ComboLabelText();
    m_sphereRadius->Initialize("(m)");

    m_luminousFluxLabel = new QLabel("Luminous Flux");

    m_luminousFlux = new ComboLabelText();
    m_luminousFlux->Initialize("(lm)");

    m_colorLabel = new QLabel("Color");

    m_colorR = new ComboLabelText();
    m_colorR->Initialize("R");

    m_colorG = new ComboLabelText();
    m_colorG->Initialize("G");

    m_colorB = new ComboLabelText();
    m_colorB->Initialize("B");

    m_line = new QWidget();
    m_line->setFixedHeight(1);
    m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_line->setStyleSheet(QString("background-color: #585858;"));

    int row = 0;
    m_gridLayout->addWidget(m_title,                        row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_sphereRadiusLabel,                     row, 0, 1, 1);
    m_gridLayout->addWidget(m_sphereRadius->GetLabelWidget(),       row, 1, 1, 1);
    m_gridLayout->addWidget(m_sphereRadius->GetTextWidget(),        row, 2, 1, 1);
    row++;

    m_gridLayout->addWidget(m_luminousFluxLabel,                     row, 0, 1, 1);
    m_gridLayout->addWidget(m_luminousFlux->GetLabelWidget(),       row, 1, 1, 1);
    m_gridLayout->addWidget(m_luminousFlux->GetTextWidget(),        row, 2, 1, 1);
    row++;

    m_gridLayout->addWidget(m_colorLabel,                     row, 0, 1, 1);
    m_gridLayout->addWidget(m_colorR->GetLabelWidget(),       row, 1, 1, 1);
    m_gridLayout->addWidget(m_colorR->GetTextWidget(),        row, 2, 1, 1);
    m_gridLayout->addWidget(m_colorG->GetLabelWidget(),       row, 3, 1, 1);
    m_gridLayout->addWidget(m_colorG->GetTextWidget(),        row, 4, 1, 1);
    m_gridLayout->addWidget(m_colorB->GetLabelWidget(),       row, 5, 1, 1);
    m_gridLayout->addWidget(m_colorB->GetTextWidget(),        row, 6, 1, 1);
    row++;

    m_gridLayout->addWidget(m_line,                         row, 0, 1, 7);

    connect(m_sphereRadius, SIGNAL(ValueChanged()), this, SLOT(SetSphereRadius()));
    connect(m_luminousFlux, SIGNAL(ValueChanged()), this, SLOT(SetLuminousFlux()));
    connect(m_colorR, SIGNAL(ValueChanged()), this, SLOT(SetColor()));
    connect(m_colorG, SIGNAL(ValueChanged()), this, SLOT(SetColor()));
    connect(m_colorB, SIGNAL(ValueChanged()), this, SLOT(SetColor()));

    m_gridLayout->setHorizontalSpacing(m_horizontalSpacing);
    m_gridLayout->setVerticalSpacing(m_verticalSpacing);

    this->setLayout(m_gridLayout);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    this->hide();
}

void SphereLightComponentPropertyEditor::edit(void *component)
{
    m_component = reinterpret_cast<SphereLightComponent*>(component);

    GetSphereRadius();
    GetLuminousFlux();
    GetColor();

    this->show();
}

void SphereLightComponentPropertyEditor::GetSphereRadius()
{
    if (!m_component)
        return;

    float sphereRadius = m_component->m_sphereRadius;

    m_sphereRadius->SetFromFloat(sphereRadius);
}

void SphereLightComponentPropertyEditor::GetLuminousFlux()
{
    if (!m_component)
        return;

    float luminousFlux = m_component->m_luminousFlux;

    m_luminousFlux->SetFromFloat(luminousFlux);
}

void SphereLightComponentPropertyEditor::GetColor()
{
    if (!m_component)
        return;

    vec4 color = m_component->m_color;

    m_colorR->SetFromFloat(color.x);
    m_colorG->SetFromFloat(color.y);
    m_colorB->SetFromFloat(color.z);
}

void SphereLightComponentPropertyEditor::SetSphereRadius()
{
    if (!m_component)
        return;

    m_component->m_sphereRadius = m_sphereRadius->GetAsFloat();
}

void SphereLightComponentPropertyEditor::SetLuminousFlux()
{
    if (!m_component)
        return;

    m_component->m_luminousFlux = m_luminousFlux->GetAsFloat();
}

void SphereLightComponentPropertyEditor::SetColor()
{
    if (!m_component)
        return;

    float x = m_colorR->GetAsFloat();
    float y = m_colorG->GetAsFloat();
    float z = m_colorB->GetAsFloat();
    vec4 color(x, y, z, 1.0f);

    m_component->m_color = color;
}

void SphereLightComponentPropertyEditor::remove()
{
    m_component = nullptr;
    this->hide();
}
