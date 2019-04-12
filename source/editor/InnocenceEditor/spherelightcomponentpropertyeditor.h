#ifndef SPHERELIGHTCOMPONENTPROPERTYEDITOR_H
#define SPHERELIGHTCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "IComponentPropertyEditor.h"
#include "combolabeltext.h"
#include "../../engine/component/SphereLightComponent.h"

class SphereLightComponentPropertyEditor : public IComponentPropertyEditor
{
    Q_OBJECT
public:
    SphereLightComponentPropertyEditor();

    void initialize() override;
    void edit(void* component) override;

    void GetSphereRadius();
    void GetLuminousFlux();
    void GetColor();

private:
    QLabel* m_sphereRadiusLabel;
    ComboLabelText* m_sphereRadius;

    QLabel* m_luminousFluxLabel;
    ComboLabelText* m_luminousFlux;

    QLabel* m_colorLabel;
    ComboLabelText* m_colorR;
    ComboLabelText* m_colorG;
    ComboLabelText* m_colorB;

    QValidator* m_validator;

    SphereLightComponent* m_component;

public slots:
    void SetSphereRadius();
    void SetLuminousFlux();
    void SetColor();

    void remove() override;
};

#endif // SPHERELIGHTCOMPONENTPROPERTYEDITOR_H
