#ifndef INNOWORLDEXPLORER_H
#define INNOWORLDEXPLORER_H

#include <QTreeWidget>
#include <QMenu>
#include <QShortcut>
#include "propertyeditor.h"

class WorldExplorer : public QTreeWidget
{
	Q_OBJECT
public:
	explicit WorldExplorer(QWidget *parent = nullptr);

	void initialize(PropertyEditor* propertyEditor);

protected:
	virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private slots:
	void onCustomContextMenuRequested(const QPoint& pos);
	void showContextMenu(QTreeWidgetItem* item, const QPoint& globalPos);
	void showGeneralMenu(const QPoint& globalPos);

	void startRename();
	void endRename();

	void addEntity();
	void deleteEntity();

	void addTransformComponent();
	void addVisibleComponent();
    void addLightComponent();
    void addCameraComponent();

	void deleteComponent();

private:
    template<class T>
    T* addComponent();

    void addChild(QTreeWidgetItem* parent, QTreeWidgetItem* child);
    void destroyComponent(Inno::Component* component);

	PropertyEditor* m_propertyEditor;

	QTreeWidgetItem* m_rootItem;
	QTreeWidgetItem* m_currentEditingItem;
	std::function<void()> f_sceneLoadingFinishCallback;
	void buildTree();
};

#endif // INNOWORLDEXPLORER_H
