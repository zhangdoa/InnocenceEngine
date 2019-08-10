#ifndef SPHERELIGHTCOMPONENTPROPERTYEDITOR_H
#define SPHERELIGHTCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "icomponentpropertyeditor.h"
#include "combolabeltext.h"
#include "../../Engine/Component/SphereLightComponent.h"

class SphereLightComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	SphereLightComponentPropertyEditor();

	void initialize() override;
	void edit(void* component) override;

	void GetSphereRadius();

private:
	QLabel* m_sphereRadiusLabel;
	ComboLabelText* m_sphereRadius;

	QValidator* m_validator;

	SphereLightComponent* m_component;

public slots:
	void SetSphereRadius();
	void remove() override;
};

#endif // SPHERELIGHTCOMPONENTPROPERTYEDITOR_H
