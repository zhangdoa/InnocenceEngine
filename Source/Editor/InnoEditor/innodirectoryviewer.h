#ifndef INNODIRECTORYVIEWER_H
#define INNODIRECTORYVIEWER_H

#include <QWidget>
#include <QSplitter>
#include "innodirectorytreeviewer.h"
#include "innodirectorylistviewer.h"

class InnoDirectoryViewer : public QSplitter
{
    Q_OBJECT
public:
    explicit InnoDirectoryViewer(QWidget *parent = nullptr);

    void Initialize();

private:
    QSplitter* m_splitter;
    InnoDirectoryTreeViewer* m_treeViewer;
    InnoDirectoryListViewer* m_listViewer;

signals:

public slots:
    void UpdateDirectoryList(QModelIndex index);
    void UpdateDirectoryTree(QModelIndex index);
};

#endif // INNODIRECTORYVIEWER_H
