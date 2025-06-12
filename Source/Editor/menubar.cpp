#include "menubar.h"
#include "../Engine/Engine.h"
#include "../Engine/Services/SceneService.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QInputDialog>
using namespace Inno;

MenuBar::MenuBar(QWidget* parent) : QMenuBar(parent)
{
    m_fileMenu = this->addMenu("File");
    m_fileMenu->addAction("Open Scene", this, SLOT(openScene()));
    m_fileMenu->addAction("Save Scene", this, SLOT(saveScene()));
}

void MenuBar::openScene()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Scene"), 
        QString::fromStdString("..//Res//Scenes//"),
        tr("InnoScene Files (*.InnoScene);;All Files (*)"));
    
    if (!fileName.isEmpty())
    {
        std::string sceneFile = fileName.toStdString();
        bool success = g_Engine->Get<SceneService>()->Load(sceneFile.c_str());
        
        if (!success)
        {
            QMessageBox::warning(this, tr("Error"), 
                tr("Failed to load scene: %1").arg(fileName));
        }
    }
}

void MenuBar::saveScene()
{
    // First, get the scene name from the user
    bool ok;
    QString sceneName = QInputDialog::getText(this, 
        tr("Save Scene"),
        tr("Scene name:"), 
        QLineEdit::Normal,
        QString::fromStdString(g_Engine->Get<SceneService>()->GetCurrentSceneName()), 
        &ok);
    
    if (!ok || sceneName.isEmpty())
    {
        return; // User cancelled or entered empty name
    }
    
    // Then open file dialog with the entered scene name
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Scene"), 
        QString::fromStdString("..//Res//Scenes//") + sceneName + ".InnoScene",
        tr("InnoScene Files (*.InnoScene);;All Files (*)"));
    
    if (!fileName.isEmpty())
    {
        std::string sceneFile = fileName.toStdString();
        bool success = g_Engine->Get<SceneService>()->Save(sceneFile.c_str());
        
        if (!success)
        {
            QMessageBox::warning(this, tr("Error"), 
                tr("Failed to save scene: %1").arg(fileName));
        }
    }
}
