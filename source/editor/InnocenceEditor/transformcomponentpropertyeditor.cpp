#include "transformcomponentpropertyeditor.h"

#include "../../engine/system/ICoreSystem.h"

extern ICoreSystem* g_pCoreSystem;

TransformComponentPropertyEditor::TransformComponentPropertyEditor()
{
}

void TransformComponentPropertyEditor::initialize()
{
    m_gridLayout = new QGridLayout();
    m_gridLayout->setMargin(4);

    m_validator = new QDoubleValidator(-2147483647, 2147483647, 4);
    m_validator->setProperty("notation", QDoubleValidator::StandardNotation);

    m_title = new QLabel("TransformComponent");
    m_title->setStyleSheet(
                "background-repeat: no-repeat;"
                "background-position: left;"
                "padding-left: 20px;"
                );

    m_posLabel = new QLabel("Position");

    m_posX = new ComboLabelText();
    m_posX->Initialize("X");

    m_posY = new ComboLabelText();
    m_posY->Initialize("Y");

    m_posZ = new ComboLabelText();
    m_posZ->Initialize("Z");

    m_rotLabel = new QLabel("Rotation");

    m_rotX = new ComboLabelText();
    m_rotX->Initialize("X");

    m_rotY = new ComboLabelText();
    m_rotY->Initialize("Y");

    m_rotZ = new ComboLabelText();
    m_rotZ->Initialize("Z");

    m_scaleLabel = new QLabel("Scale");

    m_scaleX = new ComboLabelText();
    m_scaleX->Initialize("X");

    m_scaleY = new ComboLabelText();
    m_scaleY->Initialize("Y");

    m_scaleZ = new ComboLabelText();
    m_scaleZ->Initialize("Z");

    m_line = new QWidget();
    m_line->setFixedHeight(1);
    m_line->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    m_line->setStyleSheet(QString("background-color: #585858;"));

    int row = 0;
    m_gridLayout->addWidget(m_title,                        row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_posLabel,                     row, 0, 1, 1);
    m_gridLayout->addWidget(m_posX->GetLabelWidget(),       row, 1, 1, 1);
    m_gridLayout->addWidget(m_posX->GetTextWidget(),        row, 2, 1, 1);
    m_gridLayout->addWidget(m_posY->GetLabelWidget(),       row, 3, 1, 1);
    m_gridLayout->addWidget(m_posY->GetTextWidget(),        row, 4, 1, 1);
    m_gridLayout->addWidget(m_posZ->GetLabelWidget(),       row, 5, 1, 1);
    m_gridLayout->addWidget(m_posZ->GetTextWidget(),        row, 6, 1, 1);
    row++;

    m_gridLayout->addWidget(m_rotLabel,                     row, 0, 1, 1);
    m_gridLayout->addWidget(m_rotX->GetLabelWidget(),       row, 1, 1, 1);
    m_gridLayout->addWidget(m_rotX->GetTextWidget(),        row, 2, 1, 1);
    m_gridLayout->addWidget(m_rotY->GetLabelWidget(),       row, 3, 1, 1);
    m_gridLayout->addWidget(m_rotY->GetTextWidget(),        row, 4, 1, 1);
    m_gridLayout->addWidget(m_rotZ->GetLabelWidget(),       row, 5, 1, 1);
    m_gridLayout->addWidget(m_rotZ->GetTextWidget(),        row, 6, 1, 1);
    row++;

    m_gridLayout->addWidget(m_scaleLabel,                     row, 0, 1, 1);
    m_gridLayout->addWidget(m_scaleX->GetLabelWidget(),       row, 1, 1, 1);
    m_gridLayout->addWidget(m_scaleX->GetTextWidget(),        row, 2, 1, 1);
    m_gridLayout->addWidget(m_scaleY->GetLabelWidget(),       row, 3, 1, 1);
    m_gridLayout->addWidget(m_scaleY->GetTextWidget(),        row, 4, 1, 1);
    m_gridLayout->addWidget(m_scaleZ->GetLabelWidget(),       row, 5, 1, 1);
    m_gridLayout->addWidget(m_scaleZ->GetTextWidget(),        row, 6, 1, 1);
    row++;

    m_gridLayout->addWidget(m_line,                         row, 0, 1, 7);

    connect(m_posX, SIGNAL(ValueChanged()), this, SLOT(SetPosition()));
    connect(m_posY, SIGNAL(ValueChanged()), this, SLOT(SetPosition()));
    connect(m_posZ, SIGNAL(ValueChanged()), this, SLOT(SetPosition()));
    connect(m_rotX, SIGNAL(ValueChanged()), this, SLOT(SetRotation()));
    connect(m_rotY, SIGNAL(ValueChanged()), this, SLOT(SetRotation()));
    connect(m_rotZ, SIGNAL(ValueChanged()), this, SLOT(SetRotation()));
    connect(m_scaleX, SIGNAL(ValueChanged()), this, SLOT(SetScale()));
    connect(m_scaleY, SIGNAL(ValueChanged()), this, SLOT(SetScale()));
    connect(m_scaleZ, SIGNAL(ValueChanged()), this, SLOT(SetScale()));

    m_gridLayout->setHorizontalSpacing(m_horizontalSpacing);
    m_gridLayout->setVerticalSpacing(m_verticalSpacing);

    this->setLayout(m_gridLayout);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    this->hide();
}

void TransformComponentPropertyEditor::edit(void *component)
{
    m_component = reinterpret_cast<TransformComponent*>(component);

    GetPosition();
    GetRotation();
    GetScale();

    this->show();
}

void TransformComponentPropertyEditor::GetPosition()
{
    if (!m_component)
        return;

    vec4 pos = m_component->m_localTransformVector.m_pos;

    m_posX->SetFromFloat(pos.x);
    m_posY->SetFromFloat(pos.y);
    m_posZ->SetFromFloat(pos.z);
}

void TransformComponentPropertyEditor::GetRotation()
{
    if (!m_component)
        return;

    vec4 eulerAngles = InnoMath::quatToEulerAngle(m_component->m_localTransformVector.m_rot);
    m_rotX->SetFromFloat(eulerAngles.x);
    m_rotY->SetFromFloat(eulerAngles.y);
    m_rotZ->SetFromFloat(eulerAngles.z);
}

void TransformComponentPropertyEditor::GetScale()
{
    if (!m_component)
        return;

    vec4 scale = m_component->m_localTransformVector.m_scale;
    m_scaleX->SetFromFloat(scale.x);
    m_scaleY->SetFromFloat(scale.y);
    m_scaleZ->SetFromFloat(scale.z);
}

void TransformComponentPropertyEditor::SetPosition()
{
    if (!m_component)
        return;

    float x = m_posX->GetAsFloat();
    float y = m_posY->GetAsFloat();
    float z = m_posZ->GetAsFloat();
    vec4 pos(x, y, z, 1.0f);

    m_component->m_localTransformVector.m_pos = pos;
}

void TransformComponentPropertyEditor::SetRotation()
{
    if (!m_component)
        return;

    float x = m_rotX->GetAsFloat();
    float y = m_rotY->GetAsFloat();
    float z = m_rotZ->GetAsFloat();

    auto roll = InnoMath::angleToRadian(x);
    auto pitch = InnoMath::angleToRadian(y);
    auto yaw = InnoMath::angleToRadian(z);

    m_component->m_localTransformVector.m_rot = InnoMath::eulerAngleToQuat(roll, pitch, yaw);
}

void TransformComponentPropertyEditor::SetScale()
{
    if (!m_component)
        return;

    float x = m_scaleX->GetAsFloat();
    float y = m_scaleY->GetAsFloat();
    float z = m_scaleZ->GetAsFloat();
    m_component->m_localTransformVector.m_scale = vec4(x, y, z, 1.0f);
}

void TransformComponentPropertyEditor::remove()
{
    m_component = nullptr;
    this->hide();
}
