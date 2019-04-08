#ifndef INNOPROPERTYEDITOR_H
#define INNOPROPERTYEDITOR_H

#include <QWidget>

class InnoPropertyEditor : public QWidget
{
    Q_OBJECT
public:
    explicit InnoPropertyEditor(QWidget *parent = nullptr);
    void initialize();
    void clear();

    void editComponent(int componentType, void* componentPtr);

signals:

public slots:
};

#endif // INNOPROPERTYEDITOR_H
