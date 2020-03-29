#ifndef TRANSFORMCOMPONENTPROPERTYEDITOR_H
#define TRANSFORMCOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QDoubleValidator>
#include "IComponentPropertyEditor.h"
#include "combolabeltext.h"
#include "../../Engine/Component/TransformComponent.h"

class TransformComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	TransformComponentPropertyEditor();

	void initialize() override;
	void edit(void* component) override;

	void GetPosition();
	void GetRotation();
	void GetScale();

private:
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

	TransformComponent* m_component;

public slots:
	void SetPosition();
	void SetRotation();
	void SetScale();
	void remove() override;
};

#endif // TRANSFORMCOMPONENTPROPERTYEDITOR_H
