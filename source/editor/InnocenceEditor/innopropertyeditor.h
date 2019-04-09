#ifndef INNOPROPERTYEDITOR_H
#define INNOPROPERTYEDITOR_H

#include <QWidget>
#include "icomponentpropertyeditor.h"

class InnoPropertyEditor : public QWidget
{
    Q_OBJECT
public:
    explicit InnoPropertyEditor(QWidget *parent = nullptr);
    void initialize();
    void clear();

    void editComponent(int componentType, void* componentPtr);

private:
    IComponentPropertyEditor* m_transformComponentPropertyEditor;

signals:

public slots:
};

#endif // INNOPROPERTYEDITOR_H
