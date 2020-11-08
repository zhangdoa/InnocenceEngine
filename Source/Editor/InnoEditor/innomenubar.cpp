#include "innomenubar.h"
#include "../../Engine/Interface/IModuleManager.h"

INNO_ENGINE_API extern IModuleManager* g_Engine;

InnoMenuBar::InnoMenuBar(QWidget* parent) : QMenuBar(parent)
{
    m_fileMenu = this->addMenu("File");
    m_fileMenu->addAction("Open Scene", this, SLOT(openScene()));
    m_fileMenu->addAction("Save Scene", this, SLOT(saveScene()));
}

void InnoMenuBar::openScene()
{

}

void InnoMenuBar::saveScene()
{
    g_Engine->getFileSystem()->saveScene();
}
