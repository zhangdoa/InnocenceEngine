#ifndef LIGHTCOMPONENTPROPERTYEDITOR_H
#define LIGHTCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QCheckBox>
#include <QDoubleValidator>
#include "icomponentpropertyeditor.h"
#include "combolabeltext.h"
#include "transformwidget.h"
#include "../Engine/Component/LightComponent.h"

class LightComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	LightComponentPropertyEditor();

	void initialize() override;
	void edit(void* component) override;

	void GetColor();
    void GetShape();
	void GetLuminousFlux();
	void GetColorTemperature();
	void GetUseColorTemperature();

private:
	TransformWidget* m_transformWidget;
	QLabel* m_colorLabel;
    ComboLabelText* m_colorR;
    ComboLabelText* m_colorG;
    ComboLabelText* m_colorB;

    QLabel* m_shapeLabel;
    ComboLabelText* m_shapeX;
    ComboLabelText* m_shapeY;
    ComboLabelText* m_shapeZ;
    ComboLabelText* m_shapeW;

	QLabel* m_luminousFluxLabel;
    ComboLabelText* m_LuminousFlux;

	QLabel* m_colorTemperatureLabel;
    ComboLabelText* m_colorTemperature;

    QLabel* m_useColorTemperatureLabel;
	QCheckBox* m_useColorTemperature;

    Inno::LightComponent* m_component;

public slots:
	void SetTransform();
	void SetColor();
    void SetShape();
	void SetLuminousFlux();
	void SetColorTemperature();
	void SetUseColorTemperature();

	void remove() override;
};

#endif // LIGHTCOMPONENTPROPERTYEDITOR_H
