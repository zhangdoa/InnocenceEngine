#include "innodirectorylistviewer.h"
#include <QMessageBox>

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

InnoDirectoryListViewer::InnoDirectoryListViewer(QWidget* parent) : QListView(parent)
{
    this->setAcceptDrops(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

    this->setModel(m_fileModel);

    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(InteractiveWithFile(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(OpenFileMenu(QModelIndex)));
}

void InnoDirectoryListViewer::Initialize()
{
}

QString InnoDirectoryListViewer::GetRootPath()
{
    return m_fileModel->filePath(rootIndex());
}

void InnoDirectoryListViewer::SetRootPath(QString path)
{
    this->setRootIndex(m_fileModel->setRootPath(path));
    m_rootDir = path;
}

QString InnoDirectoryListViewer::GetFilePath(QModelIndex index)
{
    return m_fileModel->filePath(index);
}

QString InnoDirectoryListViewer::GetSelectionPath()
{
    auto indices = this->selectedIndexes();
    return indices.size() != 0 ? m_fileModel->filePath(indices[0]) : "";
    }

    void InnoDirectoryListViewer::InteractiveWithFile(QModelIndex index)
    {
    auto l_fileInfo = m_fileModel->fileInfo(index);

    auto l_relativeRoot = g_pModuleManager->getFileSystem()->getWorkingDirectory() + "..//Res//";

    if (l_fileInfo.isDir())
    {
        this->SetRootPath(QString::fromStdString(l_fileInfo.canonicalFilePath().toStdString()));
    }
    else if (l_fileInfo.suffix().toStdString() == "obj"
             || l_fileInfo.suffix().toStdString() == "fbx"
             || l_fileInfo.suffix().toStdString() == "OBJ"
             || l_fileInfo.suffix().toStdString() == "FBX"
             )
    {
        QDir l_RootDir(l_relativeRoot.c_str());
        auto l_relativePath = "..//Res//" + l_RootDir.relativeFilePath(l_fileInfo.filePath());
        switch (QMessageBox::question(
                    this,
                    tr(""),
                    tr("Convert model file?"),

                    QMessageBox::Yes |
                    QMessageBox::No |
                    QMessageBox::Cancel,

                    QMessageBox::Cancel))
        {
        case QMessageBox::Yes:
            g_pModuleManager->getFileSystem()->convertModel(l_relativePath.toStdString().c_str(), "..//Res//convertedAssets//");
            break;
        case QMessageBox::No:
            g_pModuleManager->getLogSystem()->Log(LogLevel::Success, l_relativePath.toStdString().c_str());

            break;
        case QMessageBox::Cancel:
            break;
        default:
            break;
        }
    }
    else if (l_fileInfo.suffix().toStdString() == "InnoScene")
    {
        QDir l_RootDir(l_relativeRoot.c_str());
        auto l_relativePath = "..//Res//" + l_RootDir.relativeFilePath(l_fileInfo.filePath());

        switch (QMessageBox::question(
                    this,
                    tr(""),
                    tr("Load new scene?"),

                    QMessageBox::Yes |
                    QMessageBox::No |
                    QMessageBox::Cancel,

                    QMessageBox::Cancel))
        {
        case QMessageBox::Yes:
            g_pModuleManager->getFileSystem()->loadScene(l_relativePath.toStdString().c_str());
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

void InnoDirectoryListViewer::OpenFileMenu(QModelIndex index)
{
}

void InnoDirectoryListViewer::SaveScene()
{
    g_pModuleManager->getFileSystem()->saveScene();
}
