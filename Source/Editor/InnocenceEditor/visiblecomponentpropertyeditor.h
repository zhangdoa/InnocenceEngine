#ifndef VISIBLECOMPONENTPROPERTYEDITOR_H
#define VISIBLECOMPONENTPROPERTYEDITOR_H

#include <QLabel>
#include <QWidget>
#include <QGridLayout>
#include <QTableWidget>
#include <QMenu>
#include "icomponentpropertyeditor.h"
#include "../../Engine/Component/VisibleComponent.h"
#include "materialdatacomponentpropertyeditor.h"

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
    MaterialDataComponentPropertyEditor* m_MDCEditor;

	VisibleComponent* m_component;

public slots:

    void remove() override;

private slots:
    void onCustomContextMenuRequested(const QPoint& pos);
    void showContextMenu(const QPoint& globalPos);
    void tableItemClicked(int row, int column);
};

#endif // VISIBLECOMPONENTPROPERTYEDITOR_H
