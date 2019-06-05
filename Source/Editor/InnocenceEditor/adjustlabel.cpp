#include "adjustlabel.h"
#include <QMouseEvent>
#include <QApplication>
#include <QDesktopWidget>

AdjustLabel::AdjustLabel(QWidget *parent) : QLabel(parent)
{
    this->setMouseTracking(true);

    m_isMouseHovering = false;
    m_isMouseDragged = false;

    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void AdjustLabel::Initialize(QLineEdit* lineEdit)
{
    m_lineEdit = lineEdit;
}

void AdjustLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_isMouseHovering)
        MouseEntered();

    m_isMouseHovering = true;

    this->setCursor(Qt::SizeHorCursor);

    QLabel::mouseMoveEvent(event);

    if(event->buttons() == Qt::LeftButton)
        m_isMouseDragged = true;
    else
        m_isMouseDragged = false;

    Adjust();
}

void AdjustLabel::leaveEvent(QEvent* event)
{
    m_isMouseHovering = false;

    this->setCursor(Qt::ArrowCursor);

    QLabel::leaveEvent(event);
}

void AdjustLabel::MouseEntered()
{
    m_currentTexBoxValue = GetTextBoxValue();
    m_lastMousePos = GetMousePosLocal().x();
}

QPoint AdjustLabel::GetMousePosLocal()
{
    QPoint mousePos = this->mapFromGlobal(QCursor::pos());
    return mousePos;
}

float AdjustLabel::GetTextBoxValue()
{
    if (!m_lineEdit)
        return 0;

    return m_lineEdit->text().toFloat();
}

void AdjustLabel::SetTextBoxValue(float value)
{
    if (!m_lineEdit)
        return;

    m_lineEdit->setText(QString::number(value));

    emit Adjusted();
}

void AdjustLabel::RepositionMouseOnScreenEdge()
{
    QPoint mousePos = QCursor::pos();
    QRect screen = QApplication::desktop()->screenGeometry();

    if (mousePos.x() == 0)
    {
        QPoint newMousePos = QPoint(screen.width(), mousePos.y());
        QCursor::setPos(newMousePos);
        m_lastMousePos = GetMousePosLocal().x();
    }

    if (mousePos.x() == screen.width() - 1)
    {
        QPoint newMousePos = QPoint(0, mousePos.y());
        QCursor::setPos(newMousePos);
        m_lastMousePos = GetMousePosLocal().x();
    }
}

float AdjustLabel::CalculateDelta()
{
    float mousePosX = GetMousePosLocal().x();
    float mouseDelta = mousePosX - m_lastMousePos;
    m_lastMousePos = mousePosX;

    return mouseDelta;
}

void AdjustLabel::Adjust()
{
    if (!m_isMouseDragged)
        return;

    m_mouseDelta = CalculateDelta();
    RepositionMouseOnScreenEdge();

    m_currentTexBoxValue += m_mouseDelta * m_sensitivity;

    SetTextBoxValue(m_currentTexBoxValue);
}
