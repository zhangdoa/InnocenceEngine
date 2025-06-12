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
    void SetFilter(const QString& filter);

private:
    QSplitter* m_splitter;
    DirectoryTreeViewer* m_treeViewer;
    DirectoryListViewer* m_listViewer;

signals:
    void fileSelected(const QString& filePath);

public slots:
    void UpdateDirectoryList(QModelIndex index);
    void UpdateDirectoryTree(QModelIndex index);
    void onFileDoubleClicked(QModelIndex index);
};

#endif // INNODIRECTORYVIEWER_H
