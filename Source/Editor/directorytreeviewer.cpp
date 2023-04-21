#include "directorytreeviewer.h"
#include <QMessageBox>

#include "../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

DirectoryTreeViewer::DirectoryTreeViewer(QWidget *parent) : QTreeView(parent)
{
    m_dirModel = new QFileSystemModel(this);
    m_dirModel->setFilter(QDir::NoDotAndDotDot | QDir::AllDirs);
    this->setModel(m_dirModel);
    this->setColumnHidden(1, true);
    this->setColumnHidden(2, true);
    this->setColumnHidden(3, true);
}

void DirectoryTreeViewer::Initialize()
{
}

QString DirectoryTreeViewer::GetRootPath()
{
    return m_dirModel->filePath(rootIndex());
}

void DirectoryTreeViewer::SetRootPath(QString path)
{
    this->setRootIndex(m_dirModel->setRootPath(path));
}

void DirectoryTreeViewer::SetActivePath(QString path)
{
    this->scrollTo(m_dirModel->index(path), QAbstractItemView::PositionAtBottom);
}

QString DirectoryTreeViewer::GetFilePath(QModelIndex index)
{
    return m_dirModel->fileInfo(index).absoluteFilePath();
}
