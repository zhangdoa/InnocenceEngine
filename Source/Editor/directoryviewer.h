#ifndef INNODIRECTORYVIEWER_H
#define INNODIRECTORYVIEWER_H

#include <QWidget>
#include <QSplitter>
#include "directorytreeviewer.h"
#include "directorylistviewer.h"

class DirectoryViewer : public QSplitter
{
    Q_OBJECT
public:
    explicit DirectoryViewer(QWidget *parent = nullptr);

    void Initialize();

private:
    QSplitter* m_splitter;
    DirectoryTreeViewer* m_treeViewer;
    DirectoryListViewer* m_listViewer;

signals:

public slots:
    void UpdateDirectoryList(QModelIndex index);
    void UpdateDirectoryTree(QModelIndex index);
};

#endif // INNODIRECTORYVIEWER_H
