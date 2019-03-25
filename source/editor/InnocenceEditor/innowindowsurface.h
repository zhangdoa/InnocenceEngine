#ifndef INNOWINDOWSURFACE_H
#define INNOWINDOWSURFACE_H

#include <QWidget>
#include <QResizeEvent>
#include <QApplication>
#include <QTimer>

#include "../../engine/system/CoreSystem.h"
#include "../../game/GameInstance.h"

class InnoWindowSurface : public QWidget
{
    Q_OBJECT

public:
    explicit InnoWindowSurface(QWidget* parent);
    virtual ~InnoWindowSurface() override;

    void initializeEngine();

protected:
    virtual QPaintEngine* paintEngine() const { return NULL; }

    virtual void paintEvent(QPaintEvent* paintEvent) override;

    virtual void showEvent(QShowEvent* showEvent) override;

    virtual void resizeEvent(QResizeEvent* resizeEvent) override;

private:
    QTimer* m_timerUpdate;

    InnoCoreSystem* m_CoreSystem;
    GameInstance* m_GameInstance;

    void Resize(float width, float height);

public slots:
    void Update();
};

#endif // INNOWINDOWSURFACE_H
