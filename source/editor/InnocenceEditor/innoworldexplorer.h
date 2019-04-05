#ifndef INNOWORLDEXPLORER_H
#define INNOWORLDEXPLORER_H

#include <QTreeWidget>

class InnoWorldExplorer : public QTreeWidget
{
    Q_OBJECT
public:
    InnoWorldExplorer(QWidget* parent);

    void initialize();

private:
    void AddChild(QTreeWidgetItem* parent, QTreeWidgetItem* child);

    QTreeWidgetItem* m_rootItem;
};

#endif // INNOWORLDEXPLORER_H
