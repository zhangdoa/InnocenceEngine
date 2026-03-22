#ifndef VISIBLECOMPONENTPROPERTYEDITOR_H
#define VISIBLECOMPONENTPROPERTYEDITOR_H

#include <QWidget>
#include <QGridLayout>
#include "icomponentpropertyeditor.h"

class ModelComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	ModelComponentPropertyEditor() = default;

	void initialize() override {}
	void edit(void* component) override {}

public slots:
	void remove() override {}
};

#endif // VISIBLECOMPONENTPROPERTYEDITOR_H
