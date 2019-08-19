#ifndef VISIBLECOMPONENTPROPERTYEDITOR_H
#define VISIBLECOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QTableWidget>
#include "icomponentpropertyeditor.h"
#include "../../Engine/Component/VisibleComponent.h"

class VisibleComponentPropertyEditor : public IComponentPropertyEditor
{
	Q_OBJECT
public:
	VisibleComponentPropertyEditor();

	void initialize() override;
	void edit(void* component) override;

	void GetModel();

private:
    QLabel* m_modelListLabel;
    QTableWidget* m_modelList;

	VisibleComponent* m_component;

public slots:

    void remove() override;
private slots:
    void tableItemClicked(int row, int column);
};

#endif // VISIBLECOMPONENTPROPERTYEDITOR_H
