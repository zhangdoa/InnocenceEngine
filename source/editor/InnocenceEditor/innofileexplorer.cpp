#include "innofileexplorer.h"
#include "../../engine/system/ICoreSystem.h"
#include <QMessageBox>

extern ICoreSystem* g_pCoreSystem;

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

    m_rootDir = "../res/";

    SetRootDirectory(m_rootDir.toStdString());
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
    auto l_fileInfo = m_fileModel->fileInfo(index);

    if (l_fileInfo.isDir())
    {
        this->SetRootPath(QString::fromStdString(l_fileInfo.canonicalFilePath().toStdString()));
    }
    else if(l_fileInfo.suffix().toStdString() == "obj")
    {
        QDir l_RootDir(m_rootDir);
        auto l_relativePath = m_rootDir + l_RootDir.relativeFilePath(l_fileInfo.filePath());

        switch( QMessageBox::question(
                    this,
                    tr(""),
                    tr("Convert .obj file?"),

                    QMessageBox::Yes |
                    QMessageBox::No |
                    QMessageBox::Cancel,

                    QMessageBox::Cancel ) )
        {
          case QMessageBox::Yes:
            g_pCoreSystem->getFileSystem()->convertModel(l_relativePath.toStdString(), (m_rootDir + QString("convertedAssets/")).toStdString());
            break;
          case QMessageBox::No:
            break;
          case QMessageBox::Cancel:
            break;
          default:
            break;
        }
    }
    else if(l_fileInfo.suffix().toStdString() == "InnoScene")
    {
        QDir l_RootDir(m_rootDir);
        auto l_relativePath = m_rootDir + l_RootDir.relativeFilePath(l_fileInfo.filePath());

        switch( QMessageBox::question(
                    this,
                    tr(""),
                    tr("Load new scene?"),

                    QMessageBox::Yes |
                    QMessageBox::No |
                    QMessageBox::Cancel,

                    QMessageBox::Cancel ) )
        {
          case QMessageBox::Yes:
            g_pCoreSystem->getFileSystem()->loadScene(l_relativePath.toStdString());
            break;
          case QMessageBox::No:
            break;
          case QMessageBox::Cancel:
            break;
          default:
            break;
        }
    }
}
