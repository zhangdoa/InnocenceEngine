#ifndef ADJUSTLABEL_H
#define ADJUSTLABEL_H

#include <QLabel>
#include <QLineEdit>

class AdjustLabel : public QLabel
{
    Q_OBJECT
public:
    explicit AdjustLabel(QWidget* parent = nullptr);
    void Initialize(QLineEdit* lineEdit);
protected:
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void leaveEvent(QEvent* event);

private:
    void MouseEntered();
    QPoint GetMousePosLocal();
    float GetTextBoxValue();
    void SetTextBoxValue(float value);
    void RepositionMouseOnScreenEdge();
    float CalculateDelta();
    void Adjust();

    QLineEdit* m_lineEdit;
    bool m_isMouseHovering;
    bool m_isMouseDragged;
    float m_lastMousePos;
    float m_mouseDelta;
    float m_currentTexBoxValue;
    float m_sensitivity = 0.05f;

signals:
    void Adjusted();

public slots:
};
#endif // ADJUSTLABEL_H
