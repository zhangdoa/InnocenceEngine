#include "menubar.h"
#include "../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

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
    g_Engine->getSceneSystem()->saveScene();
}
