#ifndef INNOMENUBAR_H
#define INNOMENUBAR_H

#include <QMenuBar>
class InnoMenuBar : public QMenuBar
{
    Q_OBJECT
public:
    InnoMenuBar(QWidget *parent = nullptr);

private:
    QMenu* m_fileMenu;

private slots:
    void openScene();
    void saveScene();
};

#endif // INNOMENUBAR_H
