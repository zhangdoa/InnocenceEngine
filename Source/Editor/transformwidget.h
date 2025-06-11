#ifndef TRANSFORMWIDGET_H
#define TRANSFORMWIDGET_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>
#include <QDoubleValidator>
#include "combolabeltext.h"

class TransformWidget : public QWidget
{
    Q_OBJECT

public:
    TransformWidget(QWidget* parent = nullptr);
    void initialize();
    
    // Set transform values from external data
    void setPosition(float x, float y, float z);
    void setRotation(float x, float y, float z); // Euler angles in degrees
    void setScale(float x, float y, float z);
    
    // Get transform values
    void getPosition(float& x, float& y, float& z);
    void getRotation(float& x, float& y, float& z); // Euler angles in degrees
    void getScale(float& x, float& y, float& z);

private:
    QGridLayout* m_gridLayout;
    QLabel* m_title;
    QWidget* m_line;
    
    QLabel* m_posLabel;
    ComboLabelText* m_posX;
    ComboLabelText* m_posY;
    ComboLabelText* m_posZ;

    QLabel* m_rotLabel;
    ComboLabelText* m_rotX;
    ComboLabelText* m_rotY;
    ComboLabelText* m_rotZ;

    QLabel* m_scaleLabel;
    ComboLabelText* m_scaleX;
    ComboLabelText* m_scaleY;
    ComboLabelText* m_scaleZ;

    QValidator* m_validator;

signals:
    void positionChanged();
    void rotationChanged();
    void scaleChanged();

private slots:
    void onPositionChanged();
    void onRotationChanged();
    void onScaleChanged();
};

#endif // TRANSFORMWIDGET_H
