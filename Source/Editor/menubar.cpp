#include "menubar.h"
#include "../Engine/Engine.h"
#include "../Engine/Services/SceneService.h"
using namespace Inno;

MenuBar::MenuBar(QWidget* parent) : QMenuBar(parent)
{
    m_fileMenu = this->addMenu("File");
    m_fileMenu->addAction("Open Scene", this, SLOT(openScene()));
    m_fileMenu->addAction("Save Scene", this, SLOT(saveScene()));
}

void MenuBar::openScene()
{

}

void MenuBar::saveScene()
{
    g_Engine->Get<SceneService>()->Save("");
}
