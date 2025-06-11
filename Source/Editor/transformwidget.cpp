#include "transformwidget.h"

TransformWidget::TransformWidget(QWidget* parent) : QWidget(parent)
{
    m_gridLayout = nullptr;
    m_validator = nullptr;
}

void TransformWidget::initialize()
{
    m_gridLayout = new QGridLayout();
    m_gridLayout->setContentsMargins(4, 4, 4, 4);

    m_validator = new QDoubleValidator(-2147483647, 2147483647, 4);
    m_validator->setProperty("notation", QDoubleValidator::StandardNotation);

    m_title = new QLabel("Transform");
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
    m_gridLayout->addWidget(m_title, row, 0, 1, 7);
    row++;

    m_gridLayout->addWidget(m_posLabel, row, 0, 1, 1);
    m_gridLayout->addWidget(m_posX->GetLabelWidget(), row, 1, 1, 1);
    m_gridLayout->addWidget(m_posX->GetTextWidget(), row, 2, 1, 1);
    m_gridLayout->addWidget(m_posY->GetLabelWidget(), row, 3, 1, 1);
    m_gridLayout->addWidget(m_posY->GetTextWidget(), row, 4, 1, 1);
    m_gridLayout->addWidget(m_posZ->GetLabelWidget(), row, 5, 1, 1);
    m_gridLayout->addWidget(m_posZ->GetTextWidget(), row, 6, 1, 1);
    row++;

    m_gridLayout->addWidget(m_rotLabel, row, 0, 1, 1);
    m_gridLayout->addWidget(m_rotX->GetLabelWidget(), row, 1, 1, 1);
    m_gridLayout->addWidget(m_rotX->GetTextWidget(), row, 2, 1, 1);
    m_gridLayout->addWidget(m_rotY->GetLabelWidget(), row, 3, 1, 1);
    m_gridLayout->addWidget(m_rotY->GetTextWidget(), row, 4, 1, 1);
    m_gridLayout->addWidget(m_rotZ->GetLabelWidget(), row, 5, 1, 1);
    m_gridLayout->addWidget(m_rotZ->GetTextWidget(), row, 6, 1, 1);
    row++;

    m_gridLayout->addWidget(m_scaleLabel, row, 0, 1, 1);
    m_gridLayout->addWidget(m_scaleX->GetLabelWidget(), row, 1, 1, 1);
    m_gridLayout->addWidget(m_scaleX->GetTextWidget(), row, 2, 1, 1);
    m_gridLayout->addWidget(m_scaleY->GetLabelWidget(), row, 3, 1, 1);
    m_gridLayout->addWidget(m_scaleY->GetTextWidget(), row, 4, 1, 1);
    m_gridLayout->addWidget(m_scaleZ->GetLabelWidget(), row, 5, 1, 1);
    m_gridLayout->addWidget(m_scaleZ->GetTextWidget(), row, 6, 1, 1);
    row++;

    m_gridLayout->addWidget(m_line, row, 0, 1, 7);

    connect(m_posX, SIGNAL(ValueChanged()), this, SLOT(onPositionChanged()));
    connect(m_posY, SIGNAL(ValueChanged()), this, SLOT(onPositionChanged()));
    connect(m_posZ, SIGNAL(ValueChanged()), this, SLOT(onPositionChanged()));
    connect(m_rotX, SIGNAL(ValueChanged()), this, SLOT(onRotationChanged()));
    connect(m_rotY, SIGNAL(ValueChanged()), this, SLOT(onRotationChanged()));
    connect(m_rotZ, SIGNAL(ValueChanged()), this, SLOT(onRotationChanged()));
    connect(m_scaleX, SIGNAL(ValueChanged()), this, SLOT(onScaleChanged()));
    connect(m_scaleY, SIGNAL(ValueChanged()), this, SLOT(onScaleChanged()));
    connect(m_scaleZ, SIGNAL(ValueChanged()), this, SLOT(onScaleChanged()));

    m_gridLayout->setHorizontalSpacing(4);
    m_gridLayout->setVerticalSpacing(4);

    this->setLayout(m_gridLayout);
    this->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
}

void TransformWidget::setPosition(float x, float y, float z)
{
    m_posX->SetFromFloat(x);
    m_posY->SetFromFloat(y);
    m_posZ->SetFromFloat(z);
}

void TransformWidget::setRotation(float x, float y, float z)
{
    m_rotX->SetFromFloat(x);
    m_rotY->SetFromFloat(y);
    m_rotZ->SetFromFloat(z);
}

void TransformWidget::setScale(float x, float y, float z)
{
    m_scaleX->SetFromFloat(x);
    m_scaleY->SetFromFloat(y);
    m_scaleZ->SetFromFloat(z);
}

void TransformWidget::getPosition(float& x, float& y, float& z)
{
    x = m_posX->GetAsFloat();
    y = m_posY->GetAsFloat();
    z = m_posZ->GetAsFloat();
}

void TransformWidget::getRotation(float& x, float& y, float& z)
{
    x = m_rotX->GetAsFloat();
    y = m_rotY->GetAsFloat();
    z = m_rotZ->GetAsFloat();
}

void TransformWidget::getScale(float& x, float& y, float& z)
{
    x = m_scaleX->GetAsFloat();
    y = m_scaleY->GetAsFloat();
    z = m_scaleZ->GetAsFloat();
}

void TransformWidget::onPositionChanged()
{
    emit positionChanged();
}

void TransformWidget::onRotationChanged()
{
    emit rotationChanged();
}

void TransformWidget::onScaleChanged()
{
    emit scaleChanged();
}