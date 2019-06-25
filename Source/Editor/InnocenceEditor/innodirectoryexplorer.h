#ifndef INNODIRECTORYEXPLORER_H
#define INNODIRECTORYEXPLORER_H

#include <QTreeView>
#include <QFileSystemModel>
#include "innofileexplorer.h"

class InnoDirectoryExplorer : public QTreeView
{
    Q_OBJECT
public:
    explicit InnoDirectoryExplorer(QWidget *parent = nullptr);

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
