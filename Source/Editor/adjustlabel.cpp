#include "adjustlabel.h"
#include <QMouseEvent>
#include <QApplication>

AdjustLabel::AdjustLabel(QWidget *parent) : QLabel(parent)
{
    this->setMouseTracking(true);

    m_isMouseHovering = false;
    m_isMouseDragged = false;

    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
}

void AdjustLabel::Initialize(QLineEdit* lineEdit, bool isInt)
{
    m_lineEdit = lineEdit;
    m_isInt = isInt;
}

void AdjustLabel::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_isMouseHovering)
        MouseEntered();

    m_isMouseHovering = true;

    this->setCursor(Qt::SizeHorCursor);

    QLabel::mouseMoveEvent(event);

    if (event->buttons() == Qt::LeftButton)
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
    if (!m_lineEdit)
    {
        m_currentTexBoxValue = 0;
    }
    else
    {
        if(m_isInt)
        {
            m_currentTexBoxValue = m_lineEdit->text().toInt();
        }
        else
        {
            m_currentTexBoxValue = m_lineEdit->text().toFloat();
        }
    }

    m_lastMousePos = GetMousePosLocal().x();
}

QPoint AdjustLabel::GetMousePosLocal()
{
    QPoint mousePos = this->mapFromGlobal(QCursor::pos());
    return mousePos;
}

void AdjustLabel::RepositionMouseOnScreenEdge()
{
    QPoint mousePos = QCursor::pos();
    QRect screen = QApplication::primaryScreen()->geometry();

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

void AdjustLabel::Adjust()
{
    if (!m_isMouseDragged)
        return;

    float mousePosX = GetMousePosLocal().x();
    float m_mouseDelta = mousePosX - m_lastMousePos;
    m_lastMousePos = mousePosX;

    RepositionMouseOnScreenEdge();

    m_currentTexBoxValue += m_mouseDelta;

    if (!m_lineEdit)
        return;

    m_lineEdit->setText(QString::number(m_currentTexBoxValue));

    emit Adjusted();
}
