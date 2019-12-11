#include "innodirectoryviewer.h"

#include "../../Engine/ModuleManager/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_pModuleManager;

InnoDirectoryViewer::InnoDirectoryViewer(QWidget *parent) : QSplitter(parent)
{
    m_listViewer = new InnoDirectoryListViewer();
    m_treeViewer = new InnoDirectoryTreeViewer();

    m_listViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_listViewer->setWrapping(true);
    m_listViewer->setViewMode(QListView::ViewMode:: IconMode);
    m_listViewer->setResizeMode(QListView::ResizeMode::Adjust);
    m_listViewer->setUniformItemSizes(true);
    m_listViewer->setEditTriggers(QListView::EditTrigger::DoubleClicked);

    m_treeViewer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_treeViewer->setMaximumWidth(200);
    m_treeViewer->setAnimated(true);
    m_treeViewer->setEditTriggers(QTreeView::EditTrigger::NoEditTriggers);

    this->addWidget(m_treeViewer);
    this->addWidget(m_listViewer);
}

void InnoDirectoryViewer::Initialize()
{
    auto l_workingDir = g_pModuleManager->getFileSystem()->getWorkingDirectory();
    l_workingDir += "..//Res//";

    m_listViewer->SetRootPath(l_workingDir.c_str());
    m_treeViewer->SetRootPath(l_workingDir.c_str());

    connect(m_listViewer, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(UpdateDirectoryTree(QModelIndex)));
    connect(m_treeViewer, SIGNAL(clicked(QModelIndex)), this, SLOT(UpdateDirectoryList(QModelIndex)));
}

void InnoDirectoryViewer::UpdateDirectoryList(QModelIndex index)
{
    QString path = m_treeViewer->GetFilePath(index);
    m_listViewer->SetRootPath(path);
}

void InnoDirectoryViewer::UpdateDirectoryTree(QModelIndex index)
{
    QString path = m_listViewer->GetFilePath(index);
    m_treeViewer->SetActivePath(path);
}
