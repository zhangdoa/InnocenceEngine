#ifndef INNORENDERCONFIGURATOR_H
#define INNORENDERCONFIGURATOR_H

#include <QComboBox>
#include <QStandardItemModel>
#include "../../engine/system/ICoreSystem.h"

class InnoRenderConfigurator : public QComboBox
{
    Q_OBJECT
public:
    InnoRenderConfigurator(QWidget* parent = 0);
    ~InnoRenderConfigurator();
    virtual void showPopup();
    void initialize();

private slots:
    void OnItemPressed(const QModelIndex& index);

private:
    void AddCheckItem(int row, const QString& text);
    void SetRenderConfig();
    void GetRenderConfig();

    unsigned int m_rows;
    QStandardItemModel* m_model;
};

#endif // INNORENDERCONFIGURATOR_H
