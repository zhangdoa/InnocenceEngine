#ifndef VISIBLECOMPONENTPROPERTYEDITOR_H
#define VISIBLECOMPONENTPROPERTYEDITOR_H

#include <QWidget>
#include <QGridLayout>
#include "icomponentpropertyeditor.h"

// TODO Phase2-migrate: Task 13 — ModelComponent deleted; this editor is a stub until replaced with MeshComponent/MaterialComponent editors
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
