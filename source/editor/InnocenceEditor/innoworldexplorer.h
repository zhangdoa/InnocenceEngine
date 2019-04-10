#ifndef INNOWORLDEXPLORER_H
#define INNOWORLDEXPLORER_H

#include <QTreeWidget>
#include "innopropertyeditor.h"

class InnoWorldExplorer : public QTreeWidget
{
    Q_OBJECT
public:
    explicit InnoWorldExplorer(QWidget *parent = nullptr);

    void initialize(InnoPropertyEditor* propertyEditor);

protected:
    virtual void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    void addChild(QTreeWidgetItem* parent, QTreeWidgetItem* child);

    InnoPropertyEditor* m_propertyEditor;

    QTreeWidgetItem* m_rootItem;

    std::function<void()> f_sceneLoadingCallback;
    void buildTree();
};

#endif // INNOWORLDEXPLORER_H
