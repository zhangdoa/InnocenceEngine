#include "innofileexplorer.h"

InnoFileExplorer::InnoFileExplorer(QWidget* parent) : QListView(parent)
{

}

void InnoFileExplorer::initialize()
{
    this->setAcceptDrops(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_fileModel = new QFileSystemModel(this);

    m_fileModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

    this->setModel(m_fileModel);

    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(DoubleClick(QModelIndex)));

    SetRootDirectory("../../../res");
}

void InnoFileExplorer::SetRootPath(QString path)
{
    this->setRootIndex(m_fileModel->setRootPath(path));
}

QString InnoFileExplorer::GetFilePath(QModelIndex index)
{
   return m_fileModel->filePath(index);
}

void InnoFileExplorer::SetRootDirectory(const std::string &directory)
{
    QString root = QString::fromStdString(directory);
    m_fileModel->setRootPath(root);
    this->setModel(m_fileModel);
    QModelIndex index = m_fileModel->index(root);
    this->setRootIndex(index);
}

QString InnoFileExplorer::GetRootPath()
{
    return m_fileModel->filePath(rootIndex());
}

QString InnoFileExplorer::GetSelectionPath()
{
    auto indices = this->selectedIndexes();
    return indices.size() != 0 ? m_fileModel->filePath(indices[0]) : "";
}

void InnoFileExplorer::DoubleClick(QModelIndex index)
{
    std::string selectedItemPath = m_fileModel->fileInfo(index).absoluteFilePath().toStdString();

    if (m_fileModel->fileInfo(index).isDir())
    {
        this->SetRootPath(QString::fromStdString(selectedItemPath));
    }
}
