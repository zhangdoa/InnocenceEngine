#include "lightcomponentpropertyeditor.h"

LightComponentPropertyEditor::LightComponentPropertyEditor()
{
}

void LightComponentPropertyEditor::initialize()
{
	m_gridLayout = new QGridLayout();
	m_gridLayout->setMargin(4);

	m_validator = new QDoubleValidator(-2147483647, 2147483647, 4);
	m_validator->setProperty("notation", QDoubleValidator::StandardNotation);

	m_title = new QLabel("LightComponent");
	m_title->setStyleSheet(
		"background-repeat: no-repeat;"
        "background-position: left;"
	);

	m_colorLabel = new QLabel("Color");

	m_colorR = new ComboLabelText();
	m_colorR->Initialize("R");

	m_colorG = new ComboLabelText();
	m_colorG->Initialize("G");

	m_colorB = new ComboLabelText();
	m_colorB->Initialize("B");

	m_luminousFluxLabel = new QLabel("Luminous Flux");

	m_LuminousFlux = new ComboLabelText();
	m_LuminousFlux->Initialize("(lm)");

	m_colorTemperatureLabel = new QLabel("Color Temperature");

	m_colorTemperature = new ComboLabelText();
	m_colorTemperature->Initialize("(K)");

    m_useColorTemperatureLabel = new QLabel("Use Color Temperature");

    m_useColorTemperature = new QCheckBox();

	m_line = new QWidget();
	m_line->setFixedHeight(1);
	m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_line->setStyleSheet(QString("background-color: #585858;"));

	int row = 0;
	m_gridLayout->addWidget(m_title, row, 0, 1, 7);
	row++;

    m_gridLayout->addWidget(m_colorLabel, row, 0, 1, 7);
    row++;

	m_gridLayout->addWidget(m_colorR->GetLabelWidget(), row, 1, 1, 1);
	m_gridLayout->addWidget(m_colorR->GetTextWidget(), row, 2, 1, 1);
	m_gridLayout->addWidget(m_colorG->GetLabelWidget(), row, 3, 1, 1);
	m_gridLayout->addWidget(m_colorG->GetTextWidget(), row, 4, 1, 1);
	m_gridLayout->addWidget(m_colorB->GetLabelWidget(), row, 5, 1, 1);
	m_gridLayout->addWidget(m_colorB->GetTextWidget(), row, 6, 1, 1);
	row++;

    m_gridLayout->addWidget(m_luminousFluxLabel, row, 0, 1, 7);
    row++;

	m_gridLayout->addWidget(m_LuminousFlux->GetLabelWidget(), row, 1, 1, 1);
	m_gridLayout->addWidget(m_LuminousFlux->GetTextWidget(), row, 2, 1, 1);
	row++;

    m_gridLayout->addWidget(m_colorTemperatureLabel, row, 0, 1, 7);
    row++;

	m_gridLayout->addWidget(m_colorTemperature->GetLabelWidget(), row, 1, 1, 1);
	m_gridLayout->addWidget(m_colorTemperature->GetTextWidget(), row, 2, 1, 1);
	row++;

    m_gridLayout->addWidget(m_useColorTemperatureLabel, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_useColorTemperature, row, 1, 1, 1);
	row++;

	m_gridLayout->addWidget(m_line, row, 0, 1, 7);

	connect(m_colorR, SIGNAL(ValueChanged()), this, SLOT(SetColor()));
	connect(m_colorG, SIGNAL(ValueChanged()), this, SLOT(SetColor()));
	connect(m_colorB, SIGNAL(ValueChanged()), this, SLOT(SetColor()));
	connect(m_LuminousFlux, SIGNAL(ValueChanged()), this, SLOT(SetLuminousFlux()));
	connect(m_colorTemperature, SIGNAL(ValueChanged()), this, SLOT(SetColorTemperature()));
    connect(m_useColorTemperature, SIGNAL(stateChanged(int)), this, SLOT(SetUseColorTemperature()));

	m_gridLayout->setHorizontalSpacing(m_horizontalSpacing);
	m_gridLayout->setVerticalSpacing(m_verticalSpacing);

	this->setLayout(m_gridLayout);
	this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
	this->hide();
}

void LightComponentPropertyEditor::edit(void *component)
{
	m_component = reinterpret_cast<LightComponent*>(component);

	GetColor();
	GetLuminousFlux();
	GetColorTemperature();
	GetUseColorTemperature();

	this->show();
}

void LightComponentPropertyEditor::GetColor()
{
	if (!m_component)
		return;

	vec4 color = m_component->m_RGBColor;

	m_colorR->SetFromFloat(color.x);
	m_colorG->SetFromFloat(color.y);
	m_colorB->SetFromFloat(color.z);
}

void LightComponentPropertyEditor::GetLuminousFlux()
{
	if (!m_component)
		return;

	float luminousFlux = m_component->m_LuminousFlux;

	m_LuminousFlux->SetFromFloat(luminousFlux);
}

void LightComponentPropertyEditor::GetColorTemperature()
{
	if (!m_component)
		return;

	float colorTemperature = m_component->m_ColorTemperature;

	m_colorTemperature->SetFromFloat(colorTemperature);
}

void LightComponentPropertyEditor::GetUseColorTemperature()
{
	if (!m_component)
		return;

	bool useColorTemperature = m_component->m_UseColorTemperature;

    m_useColorTemperature->setChecked(useColorTemperature);
}

void LightComponentPropertyEditor::SetColor()
{
	if (!m_component)
		return;

	float x = m_colorR->GetAsFloat();
	float y = m_colorG->GetAsFloat();
	float z = m_colorB->GetAsFloat();
	vec4 color(x, y, z, 1.0f);

	m_component->m_RGBColor = color;
}

void LightComponentPropertyEditor::SetLuminousFlux()
{
	if (!m_component)
		return;

	m_component->m_LuminousFlux = m_LuminousFlux->GetAsFloat();
}

void LightComponentPropertyEditor::SetColorTemperature()
{
	if (!m_component)
		return;

	m_component->m_ColorTemperature = m_colorTemperature->GetAsFloat();
}

void LightComponentPropertyEditor::SetUseColorTemperature()
{
	if (!m_component)
		return;

	m_component->m_UseColorTemperature = m_useColorTemperature->isChecked();
}

void LightComponentPropertyEditor::remove()
{
	m_component = nullptr;
	this->hide();
}
