#include "innomenubar.h"
#include "../../Engine/Interface/IEngine.h"

using namespace Inno;
extern INNO_ENGINE_API IEngine *g_Engine;

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
    g_Engine->getSceneSystem()->saveScene();
}
