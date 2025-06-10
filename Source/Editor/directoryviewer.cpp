#include "directoryviewer.h"

#include "../Engine/Engine.h"
#include "../Engine/Common/IOService.h"
#include "../Engine/Common/LogService.h"

using namespace Inno;

DirectoryViewer::DirectoryViewer(QWidget *parent) : QSplitter(parent)
{
    m_listViewer = new DirectoryListViewer();
    m_treeViewer = new DirectoryTreeViewer();

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

void DirectoryViewer::Initialize()
{
    auto l_workingDir = g_Engine->Get<IOService>()->getWorkingDirectory();
    l_workingDir += "..//Res//";

    m_listViewer->SetRootPath(l_workingDir.c_str());
    m_treeViewer->SetRootPath(l_workingDir.c_str());

    connect(m_listViewer, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(UpdateDirectoryTree(QModelIndex)));
    connect(m_treeViewer, SIGNAL(clicked(QModelIndex)), this, SLOT(UpdateDirectoryList(QModelIndex)));
}

void DirectoryViewer::UpdateDirectoryList(QModelIndex index)
{
    QString path = m_treeViewer->GetFilePath(index);
    m_listViewer->SetRootPath(path);
}

void DirectoryViewer::UpdateDirectoryTree(QModelIndex index)
{
    QString path = m_listViewer->GetFilePath(index);
    m_treeViewer->SetActivePath(path);
}
