#include "spherelightcomponentpropertyeditor.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

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

	m_line = new QWidget();
	m_line->setFixedHeight(1);
	m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	m_line->setStyleSheet(QString("background-color: #585858;"));

	int row = 0;
	m_gridLayout->addWidget(m_title, row, 0, 1, 7);
	row++;

	m_gridLayout->addWidget(m_sphereRadiusLabel, row, 0, 1, 1);
	m_gridLayout->addWidget(m_sphereRadius->GetLabelWidget(), row, 1, 1, 1);
	m_gridLayout->addWidget(m_sphereRadius->GetTextWidget(), row, 2, 1, 1);
	row++;

	m_gridLayout->addWidget(m_line, row, 0, 1, 7);

	connect(m_sphereRadius, SIGNAL(ValueChanged()), this, SLOT(SetSphereRadius()));

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

	this->show();
}

void SphereLightComponentPropertyEditor::GetSphereRadius()
{
	if (!m_component)
		return;

	float sphereRadius = m_component->m_sphereRadius;

	m_sphereRadius->SetFromFloat(sphereRadius);
}

void SphereLightComponentPropertyEditor::SetSphereRadius()
{
	if (!m_component)
		return;

	m_component->m_sphereRadius = m_sphereRadius->GetAsFloat();
}

void SphereLightComponentPropertyEditor::remove()
{
	m_component = nullptr;
	this->hide();
}