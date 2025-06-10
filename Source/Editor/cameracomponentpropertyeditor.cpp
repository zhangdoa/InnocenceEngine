#include "cameracomponentpropertyeditor.h"

#include "../Engine/Engine.h"

using namespace Inno;


CameraComponentPropertyEditor::CameraComponentPropertyEditor()
{
}

void CameraComponentPropertyEditor::initialize()
{
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(4, 4, 4, 4);

    m_title = new QLabel("CameraComponent");
    m_title->setStyleSheet(
                "background-repeat: no-repeat;"
                "background-position: left;"
                );

    m_FOV = new ComboLabelText();
    m_FOV->Initialize("FOV");

    m_widthScale = new ComboLabelText();
    m_widthScale->Initialize("WidthScale");

    m_heightScale = new ComboLabelText();
    m_heightScale->Initialize("HeightScale");

    m_zNear = new ComboLabelText();
    m_zNear->Initialize("ZNear");

    m_zFar = new ComboLabelText();
    m_zFar->Initialize("ZFar");

    m_aperture = new ComboLabelText();
    m_aperture->Initialize("Aperture");

    m_shutterTime = new ComboLabelText();
    m_shutterTime->Initialize("ShutterTime");

    m_ISO = new ComboLabelText();
    m_ISO->Initialize("ISO");

    m_line = new QWidget();
    m_line->setFixedHeight(1);
    m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_line->setStyleSheet(QString("background-color: #585858;"));

    int row = 0;
    m_gridLayout->addWidget(m_title, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_FOV->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_FOV->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_widthScale->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_widthScale->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_heightScale->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_heightScale->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_zNear->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_zNear->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_zFar->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_zFar->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_aperture->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_aperture->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_shutterTime->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_shutterTime->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_ISO->GetLabelWidget(), row, 0, 1, 1);
    m_gridLayout->addWidget(m_ISO->GetTextWidget(), row, 1, 1, 1);
    row++;

    m_gridLayout->addWidget(m_line, row, 0, 1, 7);

    connect(m_FOV, SIGNAL(ValueChanged()), this, SLOT(SetFOV()));
    connect(m_widthScale, SIGNAL(ValueChanged()), this, SLOT(SetWidthScale()));
    connect(m_heightScale, SIGNAL(ValueChanged()), this, SLOT(SetHeightScale()));
    connect(m_zNear, SIGNAL(ValueChanged()), this, SLOT(SetZNear()));
    connect(m_zFar, SIGNAL(ValueChanged()), this, SLOT(SetZFar()));
    connect(m_aperture, SIGNAL(ValueChanged()), this, SLOT(SetAperture()));
    connect(m_shutterTime, SIGNAL(ValueChanged()), this, SLOT(SetShutterTime()));
    connect(m_ISO, SIGNAL(ValueChanged()), this, SLOT(SetISO()));

    m_gridLayout->setHorizontalSpacing(m_horizontalSpacing);
    m_gridLayout->setVerticalSpacing(m_verticalSpacing);

    this->setLayout(m_gridLayout);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    this->hide();
}

void CameraComponentPropertyEditor::edit(void *component)
{
    m_component = reinterpret_cast<CameraComponent*>(component);

    GetFOV();
    GetWidthScale();
    GetHeightScale();
    GetZNear();
    GetZFar();
    GetAperture();
    GetShutterTime();
    GetISO();

    this->show();
}

void CameraComponentPropertyEditor::GetFOV()
{
    m_FOV->SetFromFloat(m_component->m_FOVX);
}

void CameraComponentPropertyEditor::GetWidthScale()
{
    m_widthScale->SetFromFloat(m_component->m_widthScale);
}

void CameraComponentPropertyEditor::GetHeightScale()
{
    m_heightScale->SetFromFloat(m_component->m_heightScale);
}

void CameraComponentPropertyEditor::GetZNear()
{
    m_zNear->SetFromFloat(m_component->m_zNear);
}

void CameraComponentPropertyEditor::GetZFar()
{
    m_zFar->SetFromFloat(m_component->m_zFar);
}

void CameraComponentPropertyEditor::GetAperture()
{
    m_aperture->SetFromFloat(m_component->m_aperture);
}

void CameraComponentPropertyEditor::GetShutterTime()
{
    m_shutterTime->SetFromFloat(m_component->m_shutterTime);
}

void CameraComponentPropertyEditor::GetISO()
{
    m_ISO->SetFromFloat(m_component->m_ISO);
}

void CameraComponentPropertyEditor::SetFOV()
{
    m_component->m_FOVX = m_FOV->GetAsFloat();
}

void CameraComponentPropertyEditor::SetWidthScale()
{
    m_component->m_widthScale = m_widthScale->GetAsFloat();
}

void CameraComponentPropertyEditor::SetHeightScale()
{
    m_component->m_heightScale = m_heightScale->GetAsFloat();
}

void CameraComponentPropertyEditor::SetZNear()
{
    m_component->m_zNear = m_zNear->GetAsFloat();
}

void CameraComponentPropertyEditor::SetZFar()
{
    m_component->m_zFar = m_zFar->GetAsFloat();
}

void CameraComponentPropertyEditor::SetAperture()
{
    m_component->m_aperture = m_aperture->GetAsFloat();
}

void CameraComponentPropertyEditor::SetShutterTime()
{
    m_component->m_shutterTime = m_shutterTime->GetAsFloat();
}

void CameraComponentPropertyEditor::SetISO()
{
    m_component->m_ISO = m_ISO->GetAsFloat();
}

void CameraComponentPropertyEditor::remove()
{
    m_component = nullptr;
    this->hide();
}
