#ifndef POINTLIGHTCOMPONENTPROPERTYEDITOR_H
#define POINTLIGHTCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "icomponentpropertyeditor.h"
#include "combolabeltext.h"
#include "../../Engine/Component/PointLightComponent.h"

class PointLightComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	PointLightComponentPropertyEditor();

	void initialize() override;
	void edit(void* component) override;

private:
	QValidator* m_validator;

	PointLightComponent* m_component;

public slots:
	void remove() override;
};

#endif // POINTLIGHTCOMPONENTPROPERTYEDITOR_H
