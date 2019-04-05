#ifndef INNODIRECTORYEXPLORER_H
#define INNODIRECTORYEXPLORER_H

#include <QTreeView>
#include <QFileSystemModel>
#include "InnoFileExplorer.h"

class InnoDirectoryExplorer : public QTreeView
{
    Q_OBJECT
public:
    InnoDirectoryExplorer(QWidget *parent = 0);

    void initialize(InnoFileExplorer* fileExplorer);

private:
    void SetRootDirectory(const std::string& directory);

    QFileSystemModel* m_dirModel;
    InnoFileExplorer* m_fileExplorer;
signals:

public slots:
    void UpdateFileExplorer(QModelIndex index);
    void UpdateFromFileExplorer(QModelIndex index);
};

#endif // INNODIRECTORYEXPLORER_H
