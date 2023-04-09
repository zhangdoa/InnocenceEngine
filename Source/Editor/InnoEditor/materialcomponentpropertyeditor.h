#ifndef MATERIALCOMPONENTPROPERTYEDITOR_H
#define MATERIALCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "icomponentpropertyeditor.h"
#include "combolabeltext.h"
#include "../../Engine/Component/MaterialComponent.h"

class MaterialComponentPropertyEditor : public IComponentPropertyEditor
{
    Q_OBJECT
public:
    MaterialComponentPropertyEditor();

    void initialize() override;
    void edit(void* component) override;

    void GetMaterialAttributes();

private:
    QLabel* m_albedoLabel;
    ComboLabelText* m_albedoR;
    ComboLabelText* m_albedoG;
    ComboLabelText* m_albedoB;
    ComboLabelText* m_alpha;

    QLabel* m_MRATLabel;
    ComboLabelText* m_metallic;
    ComboLabelText* m_roughness;
    ComboLabelText* m_AO;
    ComboLabelText* m_thickness;

    ComboLabelText* m_shaderModel;

    MaterialComponent* m_component;

public slots:
    void SetMaterialAttributes();

    void remove() override;
};

#endif // MATERIALCOMPONENTPROPERTYEDITOR_H
