#include "innodirectoryexplorer.h"
#include <QMessageBox>

#include "../../Engine/System/ICoreSystem.h"

INNO_SYSTEM_EXPORT extern ICoreSystem* g_pCoreSystem;

InnoDirectoryExplorer::InnoDirectoryExplorer(QWidget *parent) : QTreeView(parent)
{
    m_dirModel = new QFileSystemModel(this);
    m_dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    this->setModel(m_dirModel);
    this->setColumnHidden(1, true);
    this->setColumnHidden(2, true);
    this->setColumnHidden(3, true);
}

void InnoDirectoryExplorer::initialize(InnoFileExplorer* fileExplorer)
{
    m_fileExplorer = fileExplorer;
    connect(m_fileExplorer, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(UpdateFromFileExplorer(QModelIndex)));

    auto l_workingDir = g_pCoreSystem->getFileSystem()->getWorkingDirectory();
    l_workingDir += "//res//";

    SetRootDirectory(l_workingDir);
}

void InnoDirectoryExplorer::SetRootDirectory(const std::string &directory)
{
    QString rootDir = QString::fromStdString(directory);
    m_dirModel->setRootPath(rootDir);
    QModelIndex index = m_dirModel->index(rootDir);
    this->setRootIndex(index);
}

void InnoDirectoryExplorer::UpdateFileExplorer(QModelIndex index)
{
    QString path = m_dirModel->fileInfo(index).absoluteFilePath();

    if (m_fileExplorer)
    {
        m_fileExplorer->SetRootPath(path);
    }
}

void InnoDirectoryExplorer::UpdateFromFileExplorer(QModelIndex index)
{
    QString path = m_fileExplorer->GetFilePath(index);

    this->scrollTo(m_dirModel->index(path), QAbstractItemView::PositionAtBottom);
}
