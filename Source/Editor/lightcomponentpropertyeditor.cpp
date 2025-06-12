#include "lightcomponentpropertyeditor.h"

#include "../Engine/Engine.h"

using namespace Inno;

LightComponentPropertyEditor::LightComponentPropertyEditor()
{
}

void LightComponentPropertyEditor::initialize()
{
	m_gridLayout = new QGridLayout();
	m_gridLayout->setContentsMargins(4, 4, 4, 4);

	m_title = new QLabel("LightComponent");
	m_title->setStyleSheet(
		"background-repeat: no-repeat;"
		"background-position: left;"
	);

	m_transformWidget = new TransformWidget();
	m_transformWidget->initialize();

	m_lightTypeLabel = new QLabel("Light Type");
	
	m_lightTypeComboBox = new QComboBox();
	m_lightTypeComboBox->addItem("Directional");
	m_lightTypeComboBox->addItem("Point");
	m_lightTypeComboBox->addItem("Spot");
	m_lightTypeComboBox->addItem("Sphere");
	m_lightTypeComboBox->addItem("Disk");
	m_lightTypeComboBox->addItem("Tube");
	m_lightTypeComboBox->addItem("Rectangle");

	m_colorLabel = new QLabel("Color");

	m_colorR = new ComboLabelText();
	m_colorR->Initialize("R");

	m_colorG = new ComboLabelText();
	m_colorG->Initialize("G");

	m_colorB = new ComboLabelText();
	m_colorB->Initialize("B");

	m_shapeLabel = new QLabel("Shape");

	m_shapeX = new ComboLabelText();
	m_shapeX->Initialize("X");

	m_shapeY = new ComboLabelText();
	m_shapeY->Initialize("Y");

	m_shapeZ = new ComboLabelText();
	m_shapeZ->Initialize("Z");

	m_shapeW = new ComboLabelText();
	m_shapeW->Initialize("W");

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

	m_gridLayout->addWidget(m_transformWidget, row, 0, 1, 7);
	row++;

	m_gridLayout->addWidget(m_lightTypeLabel, row, 0, 1, 7);
	row++;

	m_gridLayout->addWidget(m_lightTypeComboBox, row, 1, 1, 6);
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


	m_gridLayout->addWidget(m_shapeLabel, row, 0, 1, 7);
	row++;

	m_gridLayout->addWidget(m_shapeX->GetLabelWidget(), row, 1, 1, 1);
	m_gridLayout->addWidget(m_shapeX->GetTextWidget(), row, 2, 1, 1);
	m_gridLayout->addWidget(m_shapeY->GetLabelWidget(), row, 3, 1, 1);
	m_gridLayout->addWidget(m_shapeY->GetTextWidget(), row, 4, 1, 1);
	m_gridLayout->addWidget(m_shapeZ->GetLabelWidget(), row, 5, 1, 1);
	m_gridLayout->addWidget(m_shapeZ->GetTextWidget(), row, 6, 1, 1);
	m_gridLayout->addWidget(m_shapeW->GetLabelWidget(), row, 7, 1, 1);
	m_gridLayout->addWidget(m_shapeW->GetTextWidget(), row, 8, 1, 1);
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

	connect(m_transformWidget, SIGNAL(positionChanged()), this, SLOT(SetTransform()));
	connect(m_transformWidget, SIGNAL(rotationChanged()), this, SLOT(SetTransform()));
	connect(m_transformWidget, SIGNAL(scaleChanged()), this, SLOT(SetTransform()));
	connect(m_lightTypeComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(SetLightType()));
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

void LightComponentPropertyEditor::edit(void* component)
{
	m_component = reinterpret_cast<Inno::LightComponent*>(component);

	// Update transform widget
	m_transformWidget->setPosition(m_component->m_Transform.m_pos.x, m_component->m_Transform.m_pos.y, m_component->m_Transform.m_pos.z);

	auto eulerAngles = Math::quatToEulerAngle(m_component->m_Transform.m_rot);
	m_transformWidget->setRotation(Math::radianToAngle(eulerAngles.x), Math::radianToAngle(eulerAngles.y), Math::radianToAngle(eulerAngles.z));

	m_transformWidget->setScale(m_component->m_Transform.m_scale.x, m_component->m_Transform.m_scale.y, m_component->m_Transform.m_scale.z);

	GetLightType();
	GetColor();
	GetShape();
	GetLuminousFlux();
	GetColorTemperature();
	GetUseColorTemperature();

	this->show();
}

void LightComponentPropertyEditor::GetLightType()
{
	if (!m_component)
		return;

	int lightTypeIndex = static_cast<int>(m_component->m_LightType);
	m_lightTypeComboBox->setCurrentIndex(lightTypeIndex);
}

void LightComponentPropertyEditor::GetColor()
{
	if (!m_component)
		return;

	Vec4 color = m_component->m_RGBColor;

	m_colorR->SetFromFloat(color.x);
	m_colorG->SetFromFloat(color.y);
	m_colorB->SetFromFloat(color.z);
}

void LightComponentPropertyEditor::GetShape()
{
	if (!m_component)
		return;

	Vec4 shape = m_component->m_Shape;

	m_shapeX->SetFromFloat(shape.x);
	m_shapeY->SetFromFloat(shape.y);
	m_shapeZ->SetFromFloat(shape.z);
	m_shapeW->SetFromFloat(shape.w);
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

void LightComponentPropertyEditor::SetLightType()
{
	if (!m_component)
		return;

	int selectedIndex = m_lightTypeComboBox->currentIndex();
	m_component->m_LightType = static_cast<Inno::LightType>(selectedIndex);
}

void LightComponentPropertyEditor::SetColor()
{
	if (!m_component)
		return;

	float x = m_colorR->GetAsFloat();
	float y = m_colorG->GetAsFloat();
	float z = m_colorB->GetAsFloat();
	Vec4 color(x, y, z, 1.0f);

	m_component->m_RGBColor = color;
}

void LightComponentPropertyEditor::SetShape()
{
	if (!m_component)
		return;

	float x = m_shapeX->GetAsFloat();
	float y = m_shapeY->GetAsFloat();
	float z = m_shapeZ->GetAsFloat();
	float w = m_shapeW->GetAsFloat();
	Vec4 shape(x, y, z, w);

	m_component->m_Shape = shape;
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

void LightComponentPropertyEditor::SetTransform()
{
	if (!m_component)
		return;

	float x, y, z;
	m_transformWidget->getPosition(x, y, z);
	m_component->m_Transform.m_pos = Vec4(x, y, z, 1.0f);

	m_transformWidget->getRotation(x, y, z);
	auto roll = Math::angleToRadian(x);
	auto pitch = Math::angleToRadian(y);
	auto yaw = Math::angleToRadian(z);
	m_component->m_Transform.m_rot = Math::eulerAngleToQuat(roll, pitch, yaw);

	m_transformWidget->getScale(x, y, z);
	m_component->m_Transform.m_scale = Vec4(x, y, z, 1.0f);
}

void LightComponentPropertyEditor::remove()
{
	m_component = nullptr;
	this->hide();
}
