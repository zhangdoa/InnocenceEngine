#ifndef INNOPROPERTYEDITOR_H
#define INNOPROPERTYEDITOR_H

#include <QWidget>
#include "transformcomponentpropertyeditor.h"
#include "visiblecomponentpropertyeditor.h"
#include "lightcomponentpropertyeditor.h"
#include "directionallightcomponentpropertyeditor.h"
#include "pointlightcomponentpropertyeditor.h"
#include "spherelightcomponentpropertyeditor.h"

class InnoPropertyEditor : public QWidget
{
	Q_OBJECT
public:
	explicit InnoPropertyEditor(QWidget *parent = nullptr);
	void initialize();
	void clear();

	void editComponent(int componentType, void* componentPtr);
	void remove();

private:
	TransformComponentPropertyEditor* m_transformComponentPropertyEditor;
    VisibleComponentPropertyEditor* m_visibleComponentPropertyEditor;
	LightComponentPropertyEditor* m_lightComponentPropertyEditor;
	DirectionalLightComponentPropertyEditor* m_directionalLightComponentPropertyEditor;
	PointLightComponentPropertyEditor* m_pointLightComponentPropertyEditor;
	SphereLightComponentPropertyEditor* m_sphereLightComponentPropertyEditor;

signals:

public slots:
};

#endif // INNOPROPERTYEDITOR_H
