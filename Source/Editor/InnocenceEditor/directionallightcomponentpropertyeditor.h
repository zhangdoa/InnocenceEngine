#ifndef DIRECTIONALLIGHTCOMPONENTPROPERTYEDITOR_H
#define DIRECTIONALLIGHTCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "icomponentpropertyeditor.h"
#include "combolabeltext.h"
#include "../../Engine/Component/DirectionalLightComponent.h"

class DirectionalLightComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	DirectionalLightComponentPropertyEditor();

	void initialize() override;
	void edit(void* component) override;

private:
	QValidator* m_validator;

	DirectionalLightComponent* m_component;

public slots:
	void remove() override;
};

#endif // DIRECTIONALLIGHTCOMPONENTPROPERTYEDITOR_H
