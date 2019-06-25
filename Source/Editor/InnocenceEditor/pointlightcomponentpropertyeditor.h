#ifndef POINTLIGHTCOMPONENTPROPERTYEDITOR_H
#define POINTLIGHTCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "IComponentPropertyEditor.h"
#include "combolabeltext.h"
#include "../../Engine/Component/PointLightComponent.h"

class PointLightComponentPropertyEditor : public IComponentPropertyEditor
{
    Q_OBJECT
public:
    PointLightComponentPropertyEditor();

    void initialize() override;
    void edit(void* component) override;

    void GetLuminousFlux();
    void GetColor();

private:
    QLabel* m_luminousFluxLabel;
    ComboLabelText* m_luminousFlux;

    QLabel* m_colorLabel;
    ComboLabelText* m_colorR;
    ComboLabelText* m_colorG;
    ComboLabelText* m_colorB;

    QValidator* m_validator;

    PointLightComponent* m_component;

public slots:
    void SetLuminousFlux();
    void SetColor();

    void remove() override;
};

#endif // POINTLIGHTCOMPONENTPROPERTYEDITOR_H
