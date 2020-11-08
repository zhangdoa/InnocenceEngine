#include "innodirectorytreeviewer.h"
#include <QMessageBox>

#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_Engine;

InnoDirectoryTreeViewer::InnoDirectoryTreeViewer(QWidget *parent) : QTreeView(parent)
{
    m_dirModel = new QFileSystemModel(this);
    m_dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    this->setModel(m_dirModel);
    this->setColumnHidden(1, true);
    this->setColumnHidden(2, true);
    this->setColumnHidden(3, true);
}

void InnoDirectoryTreeViewer::Initialize()
{
}

QString InnoDirectoryTreeViewer::GetRootPath()
{
    return m_dirModel->filePath(rootIndex());
}

void InnoDirectoryTreeViewer::SetRootPath(QString path)
{
    this->setRootIndex(m_dirModel->setRootPath(path));
}

void InnoDirectoryTreeViewer::SetActivePath(QString path)
{
    this->scrollTo(m_dirModel->index(path), QAbstractItemView::PositionAtBottom);
}

QString InnoDirectoryTreeViewer::GetFilePath(QModelIndex index)
{
    return m_dirModel->fileInfo(index).absoluteFilePath();
}
