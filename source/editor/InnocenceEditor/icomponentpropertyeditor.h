#ifndef ICOMPONENTPROPERTYEDITOR_H
#define ICOMPONENTPROPERTYEDITOR_H

#include <QWidget>
#include <QLabel>
#include <QGridLayout>

class IComponentPropertyEditor : public QWidget
{
    Q_OBJECT
public:
    explicit IComponentPropertyEditor(QWidget *parent = nullptr) : QWidget(parent){}
    virtual void initialize() = 0;
    virtual void edit(void* component) = 0;

protected:
    QLabel* m_title;
    QWidget* m_line;
    QGridLayout* m_gridLayout;
    int m_verticalSpacing      = 0;
    int m_horizontalSpacing    = 0;

signals:

public slots:
    virtual void remove() = 0;
};

#endif // ICOMPONENTPROPERTYEDITOR_H
