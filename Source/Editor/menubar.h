#ifndef INNOMENUBAR_H
#define INNOMENUBAR_H

#include <QMenuBar>
class MenuBar : public QMenuBar
{
    Q_OBJECT
public:
    MenuBar(QWidget *parent = nullptr);

private:
    QMenu* m_fileMenu;

private slots:
    void openScene();
    void saveScene();
};

#endif // INNOMENUBAR_H
