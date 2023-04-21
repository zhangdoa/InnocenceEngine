#ifndef INNOPROPERTYEDITOR_H
#define INNOPROPERTYEDITOR_H

#include <QWidget>
#include "transformcomponentpropertyeditor.h"
#include "visiblecomponentpropertyeditor.h"
#include "lightcomponentpropertyeditor.h"
#include "cameracomponentpropertyeditor.h"

class PropertyEditor : public QWidget
{
	Q_OBJECT
public:
	explicit PropertyEditor(QWidget *parent = nullptr);
	void initialize();
	void clear();

	void editComponent(int componentType, void* componentPtr);
	void remove();

private:
	TransformComponentPropertyEditor* m_transformComponentPropertyEditor;
    VisibleComponentPropertyEditor* m_visibleComponentPropertyEditor;
	LightComponentPropertyEditor* m_lightComponentPropertyEditor;
    CameraComponentPropertyEditor* m_cameraComponentPropertyEditor;
signals:

public slots:
};

#endif // INNOPROPERTYEDITOR_H
