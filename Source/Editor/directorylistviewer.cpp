#include "directorylistviewer.h"
#include <QMessageBox>

#include "../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

DirectoryListViewer::DirectoryListViewer(QWidget* parent) : QListView(parent)
{
    this->setAcceptDrops(true);
    this->setEditTriggers(QAbstractItemView::NoEditTriggers);

    m_fileModel = new QFileSystemModel(this);
    m_fileModel->setFilter(QDir::Files | QDir::AllDirs | QDir::NoDotAndDotDot);

    this->setModel(m_fileModel);

    connect(this, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(InteractiveWithFile(QModelIndex)));
    connect(this, SIGNAL(clicked(QModelIndex)), this, SLOT(OpenFileMenu(QModelIndex)));
}

void DirectoryListViewer::Initialize()
{
}

QString DirectoryListViewer::GetRootPath()
{
    return m_fileModel->filePath(rootIndex());
}

void DirectoryListViewer::SetRootPath(QString path)
{
    this->setRootIndex(m_fileModel->setRootPath(path));
    m_rootDir = path;
}

QString DirectoryListViewer::GetFilePath(QModelIndex index)
{
    return m_fileModel->filePath(index);
}

QString DirectoryListViewer::GetSelectionPath()
{
    auto indices = this->selectedIndexes();
    return indices.size() != 0 ? m_fileModel->filePath(indices[0]) : "";
    }

    void DirectoryListViewer::InteractiveWithFile(QModelIndex index)
    {
    auto l_fileInfo = m_fileModel->fileInfo(index);

    auto l_relativeRoot = g_Engine->getFileSystem()->getWorkingDirectory() + "..//Res//";

    if (l_fileInfo.isDir())
    {
        this->SetRootPath(QString::fromStdString(l_fileInfo.canonicalFilePath().toStdString()));
    }
    else if (l_fileInfo.suffix().toStdString() == "obj"
             || l_fileInfo.suffix().toStdString() == "fbx"
             || l_fileInfo.suffix().toStdString() == "gltf"
             || l_fileInfo.suffix().toStdString() == "fbx"
             || l_fileInfo.suffix().toStdString() == "md5mesh"
             || l_fileInfo.suffix().toStdString() == "FBX"
             || l_fileInfo.suffix().toStdString() == "GLTF"
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
            g_Engine->getAssetSystem()->convertModel(l_relativePath.toStdString().c_str(), "..//Res//convertedAssets//");
            break;
        case QMessageBox::No:
            g_Engine->getLogSystem()->Log(LogLevel::Success, l_relativePath.toStdString().c_str());

            break;
        case QMessageBox::Cancel:
            break;
        default:
            break;
        }
    }
    else if (l_fileInfo.suffix().toStdString() == "Scene")
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
            g_Engine->getSceneSystem()->loadScene(l_relativePath.toStdString().c_str());
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

void DirectoryListViewer::OpenFileMenu(QModelIndex index)
{
}

void DirectoryListViewer::SaveScene()
{
    g_Engine->getSceneSystem()->saveScene();
}
